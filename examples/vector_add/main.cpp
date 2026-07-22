#include "vkr.hh"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

constexpr uint32_t ElementCount = 1024;
constexpr uint32_t LocalSize = 64;

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

struct alignas(16) VectorAddParams {
  uint32_t elementCount{0};
};

} // namespace

class VectorAddApp final : public vkr::exec::ComputeApplication {
private:
  std::vector<float> a_{};
  std::vector<float> b_{};
  std::vector<float> c_{};
  std::unique_ptr<vkr::resource::StorageBuffer<float>> input_a_{};
  std::unique_ptr<vkr::resource::StorageBuffer<float>> input_b_{};
  std::unique_ptr<vkr::resource::StorageBuffer<float>> output_c_{};
  std::unique_ptr<vkr::resource::UniformBuffer<VectorAddParams>> params_{};

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
    params_ = std::make_unique<vkr::resource::UniformBuffer<VectorAddParams>>(
        *device);

    input_a_->write(a_);
    input_b_->write(b_);
    output_c_->write(c_);
    params_->update(VectorAddParams{.elementCount = ElementCount});
  }

  void buildGraph() override {
    vkr::exec::ComputePassDesc passDesc{};
    passDesc.descriptorBindings = {storageBinding(0), storageBinding(1),
                                   storageBinding(2), uniformBinding(3)};
    passDesc.descriptorWrites = {vkr::pipeline::DescriptorSetWriteDesc{
        .setIndex = 0,
        .buffers = {storageWrite(0, *input_a_), storageWrite(1, *input_b_),
                    storageWrite(2, *output_c_),
                    vkr::pipeline::DescriptorBufferWriteDesc::one(
                        3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        params_->descriptorInfo())}}};
    passDesc.pipeline = vkr::pipeline::ComputePipelineDesc{
        .name = "vector_add",
        .shader = vkr::resource::ShaderModuleDesc::computeGlslFile(
            assetSystem->resolveApp("shaders/vector_add.comp").string())};
    passDesc.dispatch = vkr::exec::ComputeDispatchDesc{
        .groupCountX = (ElementCount + LocalSize - 1) / LocalSize,
        .groupCountY = 1,
        .groupCountZ = 1};

    auto &pass = graph->addPass(*executor, *device);
    pass.setName("vector_add")
        .setReads({"input_a", "input_b"})
        .setWrites({"output_c"});
    pass.update(passDesc);
  }

  void afterExecute() override {
    output_c_->read(c_);

    for (uint32_t i = 0; i < ElementCount; ++i) {
      const float expected = a_[i] + b_[i];
      if (std::fabs(c_[i] - expected) > 0.0001F) {
        throw std::runtime_error("vector_add validation failed at index " +
                                 std::to_string(i));
      }
    }

    std::cout << "vector_add passed: " << ElementCount << " elements\n";

    for (uint32_t i = 0; i < 16; ++i) {
      std::cout << a_[i] << " + " << b_[i] << " = " << c_[i] << std::endl;
    }
  }

  void configure() override { ctx.instance.name = "vector_add"; }
};

auto main() -> int {
  try {
    VectorAddApp app{};
    app.run();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "vector_add failed: " << e.what() << '\n';
    return 1;
  }
}
