#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>
#include <vkr.hh>

namespace {

constexpr uint32_t ElementCount = 1U << 18U;
constexpr uint32_t LocalSize = 64;
constexpr uint32_t Iterations = 32;

auto storageBinding(uint32_t binding) -> vkr::pipeline::DescriptorBinding {
  return vkr::pipeline::DescriptorBinding{
      .layout = {binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                 VK_SHADER_STAGE_COMPUTE_BIT}};
}

auto uniformBinding(uint32_t binding) -> vkr::pipeline::DescriptorBinding {
  return vkr::pipeline::DescriptorBinding{
      .layout = {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                 VK_SHADER_STAGE_COMPUTE_BIT}};
}

auto storageWrite(uint32_t binding,
                  const vkr::resource::StorageBuffer<float> &buffer)
    -> vkr::pipeline::DescriptorBufferWriteDesc {
  return vkr::pipeline::DescriptorBufferWriteDesc::storage(
      binding, buffer.descriptorInfo(0, buffer.bufferSize()));
}

struct alignas(16) VectorOpsParams {
  uint32_t elementCount{0};
  uint32_t iterations{0};
};

struct TimingStats {
  double minMs{0.0};
  double meanMs{0.0};
  double medianMs{0.0};
  double maxMs{0.0};
};

auto timingStats(std::vector<double> samples) -> TimingStats {
  if (samples.empty()) {
    return {};
  }

  std::sort(samples.begin(), samples.end());
  const double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
  double median = samples[samples.size() / 2];
  if (samples.size() % 2 == 0) {
    median = (samples[(samples.size() / 2) - 1] + median) * 0.5;
  }

  return TimingStats{
      .minMs = samples.front(),
      .meanMs = sum / static_cast<double>(samples.size()),
      .medianMs = median,
      .maxMs = samples.back(),
  };
}

auto nonlinearOp(float a, float b, uint32_t iterations) -> float {
  float x = a * 0.000123F + b * 0.000071F;
  float y = b * 0.000097F + 0.25F;

  for (uint32_t i = 0; i < iterations; ++i) {
    x = std::sin(x) * 0.73F + std::cos(y) * 0.19F +
        std::sqrt(std::fabs(x * y) + 0.001F);
    y = std::sin(y + x * 0.13F) * 0.61F + std::cos(x - y * 0.07F) * 0.29F;
    x = std::clamp(x, -8.0F, 8.0F);
    y = std::clamp(y, -8.0F, 8.0F);
  }

  return x + y * 0.5F;
}

} // namespace

class VectorOpsApp final : public vkr::exec::ComputeApplication {
private:
  std::vector<float> a_{};
  std::vector<float> b_{};
  std::vector<float> c_{};
  std::unique_ptr<vkr::resource::StorageBuffer<float>> input_a_{};
  std::unique_ptr<vkr::resource::StorageBuffer<float>> input_b_{};
  std::unique_ptr<vkr::resource::StorageBuffer<float>> output_c_{};
  std::unique_ptr<vkr::resource::UniformBuffer<VectorOpsParams>> params_{};

  void createResources() override {
    a_.resize(ElementCount);
    b_.resize(ElementCount);
    c_.assign(ElementCount, 0.0F);

    for (uint32_t i = 0; i < ElementCount; ++i) {
      a_[i] = static_cast<float>(i);
      b_[i] = static_cast<float>(ElementCount - i);
    }

    const auto storageDesc =
        vkr::resource::StorageBufferDesc::hostVisible(ElementCount);
    input_a_ = std::make_unique<vkr::resource::StorageBuffer<float>>(
        *device, storageDesc);
    input_b_ = std::make_unique<vkr::resource::StorageBuffer<float>>(
        *device, storageDesc);
    output_c_ = std::make_unique<vkr::resource::StorageBuffer<float>>(
        *device, storageDesc);
    params_ = std::make_unique<vkr::resource::UniformBuffer<VectorOpsParams>>(
        *device);

    input_a_->write(a_);
    input_b_->write(b_);
    output_c_->write(c_);
    params_->update({ElementCount, Iterations});
  }

  void buildGraph() override {
    vkr::exec::ComputePassDesc passDesc{};
    passDesc.descriptorBindings = {storageBinding(0), storageBinding(1),
                                   storageBinding(2), uniformBinding(3)};
    passDesc.descriptorWrites = {vkr::pipeline::DescriptorSetWriteDesc{
        .setIndex = 0,
        .buffers = {storageWrite(0, *input_a_), storageWrite(1, *input_b_),
                    storageWrite(2, *output_c_),
                    vkr::pipeline::DescriptorBufferWriteDesc::uniform(
                        3, params_->descriptorInfo())}}};
    passDesc.pipeline = vkr::pipeline::ComputePipelineDesc{
        .name = "vector_ops",
        .shader = vkr::resource::ShaderModuleDesc::computeGlslFile(
            assetSystem->resolveApp("shaders/vector_ops.comp").string())};
    passDesc.dispatch =
        vkr::exec::ComputeDispatchDesc::dispatch1D(LocalSize, ElementCount);

    auto &pass = graph->addPass(*executor, *device);
    pass.setName("vector_ops")
        .setReads({"input_a", "input_b"})
        .setWrites({"output_c"});
    pass.update(passDesc);
  }

  void afterExecute() override {
    output_c_->read(c_);

    std::vector<float> cpuResult(ElementCount, 0.0F);
    auto runCpuVectorOps = [&]() -> void {
      for (uint32_t i = 0; i < ElementCount; ++i) {
        cpuResult[i] = nonlinearOp(a_[i], b_[i], Iterations);
      }
    };

    for (uint32_t i = 0; i < ctx.profiler.warmupFrames; ++i) {
      runCpuVectorOps();
    }

    std::vector<double> cpuSamples{};
    cpuSamples.reserve(ctx.profiler.captureFrames);
    for (uint32_t i = 0; i < ctx.profiler.captureFrames; ++i) {
      timer->reset();
      runCpuVectorOps();
      timer->update();
      cpuSamples.push_back(timer->elapsedMilliseconds());
    }

    const auto cpuStats = timingStats(cpuSamples);

    for (uint32_t i = 0; i < ElementCount; ++i) {
      if (std::fabs(c_[i] - cpuResult[i]) > 0.02F) {
        throw std::runtime_error("vector_ops validation failed at index " +
                                 std::to_string(i));
      }
    }

    std::cout << "vector_ops passed: " << ElementCount << " elements, "
              << Iterations << " nonlinear iterations\n";
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "captures:       " << ctx.profiler.captureFrames
              << " profiled, " << ctx.profiler.warmupFrames << " warmup\n";
    std::cout << "cpu vector_ops: min=" << cpuStats.minMs
              << " ms, mean=" << cpuStats.meanMs
              << " ms, median=" << cpuStats.medianMs
              << " ms, max=" << cpuStats.maxMs << " ms\n";

    const vkr::exec::ProfileSample *gpuSample = nullptr;
    for (const auto &sample : profileReport.gpuSamples) {
      if (sample.name == "vector_ops") {
        gpuSample = &sample;
        break;
      }
    }

    if (gpuSample != nullptr && gpuSample->milliseconds > 0.0) {
      std::cout << "gpu dispatch:   min=" << gpuSample->minMilliseconds
                << " ms, mean=" << gpuSample->milliseconds
                << " ms, median=" << gpuSample->medianMilliseconds
                << " ms, max=" << gpuSample->maxMilliseconds << " ms\n";
      std::cout << "gpu speedup:    mean="
                << cpuStats.meanMs / gpuSample->milliseconds << "x, median="
                << cpuStats.medianMs / gpuSample->medianMilliseconds << "x\n";
    } else {
      std::cout << "gpu dispatch:   unavailable\n";
      std::cout << "gpu speedup:    unavailable\n";
    }

    for (uint32_t i = 0; i < 8; ++i) {
      std::cout << "op(" << a_[i] << ", " << b_[i] << ") = " << c_[i]
                << std::endl;
    }
  }

  void configure() override {
    ctx.instance.name = "vector_ops";
    ctx.profiler.enableGpuTimestamps = true;
    ctx.profiler.warmupFrames = 8;
    ctx.profiler.captureFrames = 16;
  }
};

auto main() -> int {
  try {
    VectorOpsApp app{};
    app.run();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "vector_ops failed: " << e.what() << '\n';
    return 1;
  }
}
