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

constexpr uint32_t M{1U << 10};
constexpr uint32_t N{1U << 10};
constexpr uint32_t K{1U << 10};
constexpr uint32_t WarmupRuns = 2;
constexpr uint32_t MeasuredRuns = 5;
constexpr uint32_t LocalSize = 16;

struct alignas(16) MMulParams {
  uint32_t M;
  uint32_t K;
  uint32_t N;
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

} // namespace

class MMulApplication final : public vkr::exec::ComputeApplication {
  std::vector<float> A_{};
  std::vector<float> B_{};
  std::vector<float> C_{};
  std::unique_ptr<vkr::resource::StorageBuffer<float>> input_A_{};
  std::unique_ptr<vkr::resource::StorageBuffer<float>> input_B_{};
  std::unique_ptr<vkr::resource::StorageBuffer<float>> output_C_{};
  std::unique_ptr<vkr::resource::UniformBuffer<MMulParams>> params_{};

  void createResources() override {
    A_.resize(M * K);
    B_.resize(K * N);
    C_.assign(M * N, 0.0f);

    for (uint32_t m = 0; m < M; m++) {
      for (uint32_t k = 0; k < K; k++) {
        A_[m * K + k] = static_cast<float>((m * 13u + k * 7u) % 257u) / 257.0f;
      }
    }

    for (uint32_t k = 0; k < K; k++) {
      for (uint32_t n = 0; n < N; n++) {
        B_[k * N + n] = static_cast<float>((k * 11u + n * 5u) % 263u) / 263.0f;
      }
    }

    vkr::resource::StorageBufferDesc descA{}, descB{}, descC{};
    descA.elements(M * K)
        .hostVisible()
        .storage()
        .readonly()
        .transfer()
        .mapped();

    descB.elements(K * N)
        .hostVisible()
        .storage()
        .readonly()
        .transfer()
        .mapped();

    descC.elements(M * N)
        .hostVisible()
        .storage()
        .writeonly()
        .transfer()
        .mapped();

    input_A_ =
        std::make_unique<vkr::resource::StorageBuffer<float>>(*device, descA);

    input_B_ =
        std::make_unique<vkr::resource::StorageBuffer<float>>(*device, descB);

    output_C_ =
        std::make_unique<vkr::resource::StorageBuffer<float>>(*device, descC);

    params_ =
        std::make_unique<vkr::resource::UniformBuffer<MMulParams>>(*device);

    input_A_->write(A_);
    input_B_->write(B_);
    output_C_->write(C_);
    params_->update({M, K, N});
  }

  void buildGraph() override {
    vkr::exec::ComputePassDesc passDesc{};
    passDesc.storage(0, *input_A_)
        .storage(1, *input_B_)
        .storage(2, *output_C_)
        .uniform(3, *params_)
        .shader("mmul",
#ifdef VKR_HAS_SLANG
                vkr::resource::ShaderModuleDesc::computeSlangFile(
                    assetSystem->resolveApp("shaders/mmul.slang").string()))
#else
                vkr::resource::ShaderModuleDesc::computeGlslFile(
                    assetSystem->resolveApp("shaders/mmul.comp").string()))
#endif
        .dispatch2D(LocalSize, M, LocalSize, N);

    auto &pass = graph->addPass<vkr::exec::ComputePass>(*executor, *device);
    pass.setName("mmul")
        .setReads({"input_A", "input_B"})
        .setWrites({"output_C"});
    pass.update(passDesc);
  }

  void afterExecute() override {
    output_C_->read(C_);

    std::vector<float> cpuResult(M * N, 0.0f);
    auto runCpuMMul = [&]() -> void {
      for (uint32_t m = 0; m < M; m++) {
        for (uint32_t n = 0; n < N; n++) {
          float sum = 0.0f;
          for (uint32_t k = 0; k < K; k++) {
            sum += A_[m * K + k] * B_[k * N + n];
          }
          cpuResult[m * N + n] = sum;
        }
      }
    };

    std::vector<double> cpuSamples{};
    cpuSamples.reserve(MeasuredRuns);
    for (uint32_t i = 0; i < MeasuredRuns; ++i) {
      timer->reset();
      runCpuMMul();
      timer->update();
      cpuSamples.push_back(timer->elapsedMilliseconds());
    }

    const auto cpuStats = timingStats(cpuSamples);

    for (uint32_t i = 0; i < M * N; ++i) {
      if (std::fabs(C_[i] - cpuResult[i]) > 0.01f) {
        throw std::runtime_error("mmul validation failed at index " +
                                 std::to_string(i));
      }
    }

    std::cout << "mmul passed: " << N << "x" << M << "x" << K << " elements\n";
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "gpu captures:   " << MeasuredRuns << " profiled, "
              << WarmupRuns << " warmup\n";
    std::cout << "cpu mmul: min=" << cpuStats.minMs
              << " ms, mean=" << cpuStats.meanMs
              << " ms, median=" << cpuStats.medianMs
              << " ms, max=" << cpuStats.maxMs << " ms\n";

    const vkr::exec::ProfileSample *gpuSample = nullptr;
    for (const auto &sample : profileReport.gpuSamples) {
      if (sample.name == "mmul") {
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
  }

  void configure() override {
    ctx.instance.name = "mmul";
    ctx.profiler.enableGpuTimestamps = true;
  }
};

VKR_COMP_APP_BENCHMARK(MMulApplication, WarmupRuns, MeasuredRuns)
