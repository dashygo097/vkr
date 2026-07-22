#pragma once

#include "vkr/core/device.hh"
#include "vkr/exec/compute/executor.hh"
#include "vkr/pipeline/compute_pipeline.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/resource/buffer/storage_buffer.hh"
#include "vkr/resource/buffer/uniform_buffer.hh"
#include "vkr/resource/shader/module.hh"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace vkr::exec {

struct ComputeDispatchDesc {
  uint32_t groupCountX{1};
  uint32_t groupCountY{1};
  uint32_t groupCountZ{1};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return groupCountX > 0 && groupCountY > 0 && groupCountZ > 0;
  }

  [[nodiscard]] static auto dispatch1D(uint32_t LocalSize,
                                       uint32_t ElementCount)
      -> ComputeDispatchDesc {
    return {
        .groupCountX = (ElementCount + LocalSize - 1) / LocalSize,
        .groupCountY = 1,
        .groupCountZ = 1,
    };
  }
};

struct ComputePassDesc {
  std::vector<pipeline::DescriptorBinding> descriptorBindings{};
  pipeline::DescriptorPoolDesc descriptorPool{};
  uint32_t descriptorSetCount{1};
  std::vector<pipeline::DescriptorSetWriteDesc> descriptorWrites{};
  pipeline::ComputePipelineDesc pipeline{};
  ComputeDispatchDesc dispatch{};

  template <typename ElementType>
  auto storage(uint32_t binding,
               const resource::StorageBuffer<ElementType> &buffer,
               uint32_t setIndex = 0) -> ComputePassDesc & {
    descriptorBindings.push_back(pipeline::DescriptorBinding{
        .layout = {binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                   VK_SHADER_STAGE_COMPUTE_BIT}});
    descriptorWrite(setIndex).buffers.push_back(
        pipeline::DescriptorBufferWriteDesc::storage(
            binding, buffer.descriptorInfo(0, buffer.bufferSize())));
    return *this;
  }

  template <typename UniformType>
  auto uniform(uint32_t binding,
               const resource::UniformBuffer<UniformType> &buffer,
               uint32_t setIndex = 0) -> ComputePassDesc & {
    descriptorBindings.push_back(pipeline::DescriptorBinding{
        .layout = {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                   VK_SHADER_STAGE_COMPUTE_BIT}});
    descriptorWrite(setIndex).buffers.push_back(
        pipeline::DescriptorBufferWriteDesc::uniform(binding,
                                                     buffer.descriptorInfo()));
    return *this;
  }

  auto shader(std::string name, resource::ShaderModuleDesc shaderDesc)
      -> ComputePassDesc & {
    pipeline.name = std::move(name);
    pipeline.shader = std::move(shaderDesc);
    return *this;
  }

  auto dispatch1D(uint32_t localSize, uint32_t elementCount)
      -> ComputePassDesc & {
    dispatch = ComputeDispatchDesc::dispatch1D(localSize, elementCount);
    return *this;
  }

private:
  auto descriptorWrite(uint32_t setIndex) -> pipeline::DescriptorSetWriteDesc & {
    for (auto &write : descriptorWrites) {
      if (write.setIndex == setIndex) {
        return write;
      }
    }

    descriptorWrites.push_back(
        pipeline::DescriptorSetWriteDesc::forSet(setIndex));
    return descriptorWrites.back();
  }
};

class ComputePass final {
public:
  explicit ComputePass(ComputeExecutor &executor, const core::Device &device);
  ~ComputePass();

  ComputePass(const ComputePass &) = delete;
  auto operator=(const ComputePass &) -> ComputePass & = delete;

  [[nodiscard]] auto name() const noexcept -> const std::string & {
    return name_;
  }

  [[nodiscard]] auto reads() const noexcept
      -> const std::vector<std::string> & {
    return reads_;
  }

  [[nodiscard]] auto writes() const noexcept
      -> const std::vector<std::string> & {
    return writes_;
  }

  auto setName(std::string name) -> ComputePass & {
    name_ = std::move(name);
    return *this;
  }

  auto setReads(std::vector<std::string> reads) -> ComputePass & {
    reads_ = std::move(reads);
    return *this;
  }

  auto setWrites(std::vector<std::string> writes) -> ComputePass & {
    writes_ = std::move(writes);
    return *this;
  }

  auto read(std::string resource) -> ComputePass & {
    reads_.push_back(std::move(resource));
    return *this;
  }

  auto write(std::string resource) -> ComputePass & {
    writes_.push_back(std::move(resource));
    return *this;
  }

  void create();
  void destroy();
  void update(const ComputePassDesc &desc);
  void record();

private:
  // dependencies
  ComputeExecutor &executor_;
  const core::Device &device_;

  // components
  ComputePassDesc desc_{};
  std::unique_ptr<pipeline::DescriptorPool> descriptor_pool_{};
  std::unique_ptr<pipeline::DescriptorSetLayout> descriptor_layout_{};
  std::unique_ptr<pipeline::DescriptorSets> descriptor_sets_{};
  std::unique_ptr<pipeline::ComputePipeline> pipeline_{};
  std::string name_{};
  std::vector<std::string> reads_{};
  std::vector<std::string> writes_{};

  // helpers
  void createDescriptors();
  void createPipeline();

  [[nodiscard]] auto descriptorSetCount() const -> uint32_t;
  [[nodiscard]] auto descriptorPoolDesc(uint32_t setCount) const
      -> pipeline::DescriptorPoolDesc;
  [[nodiscard]] auto descriptorBindings() const
      -> std::vector<pipeline::DescriptorBinding>;
  void validateDescriptorWrites(uint32_t setCount) const;
};

} // namespace vkr::exec
