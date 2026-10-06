#pragma once

#include "vkr/core/device.hh"
#include "vkr/exec/compute/executor.hh"
#include "vkr/exec/pass.hh"
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

  [[nodiscard]] static auto dispatchX(uint32_t X) -> ComputeDispatchDesc {
    return {
        .groupCountX = X,
        .groupCountY = 1,
        .groupCountZ = 1,
    };
  }

  [[nodiscard]] static auto dispatchXY(uint32_t X, uint32_t Y)
      -> ComputeDispatchDesc {
    return {
        .groupCountX = X,
        .groupCountY = Y,
        .groupCountZ = 1,
    };
  }

  [[nodiscard]] static auto dispatchXYZ(uint32_t X, uint32_t Y, uint32_t Z)
      -> ComputeDispatchDesc {
    return {
        .groupCountX = X,
        .groupCountY = Y,
        .groupCountZ = Z,
    };
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

  [[nodiscard]] static auto
  dispatch2D(uint32_t LocalSizeX, uint32_t ElementCountX, uint32_t LocalSizeY,
             uint32_t ElementCountY) -> ComputeDispatchDesc {
    return {
        .groupCountX = (ElementCountX + LocalSizeX - 1) / LocalSizeX,
        .groupCountY = (ElementCountY + LocalSizeY - 1) / LocalSizeY,
        .groupCountZ = 1,
    };
  }

  [[nodiscard]] static auto
  dispatch3D(uint32_t LocalSizeX, uint32_t ElementCountX, uint32_t LocalSizeY,
             uint32_t ElementCountY, uint32_t LocalSizeZ,
             uint32_t ElementCountZ) -> ComputeDispatchDesc {
    return {
        .groupCountX = (ElementCountX + LocalSizeX - 1) / LocalSizeX,
        .groupCountY = (ElementCountY + LocalSizeY - 1) / LocalSizeY,
        .groupCountZ = (ElementCountZ + LocalSizeZ - 1) / LocalSizeZ,
    };
  }
};

struct ComputePassDesc {
  std::vector<pipeline::DescriptorBinding> descriptorBindings{};
  pipeline::DescriptorPoolDesc descriptorPool{};
  uint32_t descriptorSetCount{1};
  std::vector<pipeline::DescriptorSetWrite> descriptorWrites{};
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
        pipeline::DescriptorBufferWrite::storage(
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
        pipeline::DescriptorBufferWrite::uniform(binding,
                                                     buffer.descriptorInfo()));
    return *this;
  }

  auto shader(std::string name, resource::ShaderModuleDesc shaderDesc)
      -> ComputePassDesc & {
    pipeline.name = std::move(name);
    pipeline.shader = std::move(shaderDesc);
    return *this;
  }

  auto dispatchX(uint32_t X) -> ComputePassDesc & {
    dispatch = ComputeDispatchDesc::dispatchX(X);
    return *this;
  }

  auto dispatchXY(uint32_t X, uint32_t Y) -> ComputePassDesc & {
    dispatch = ComputeDispatchDesc::dispatchXY(X, Y);
    return *this;
  }

  auto dispatchXYZ(uint32_t X, uint32_t Y, uint32_t Z) -> ComputePassDesc & {
    dispatch = ComputeDispatchDesc::dispatchXYZ(X, Y, Z);
    return *this;
  }

  auto dispatch1D(uint32_t localSize, uint32_t elementCount)
      -> ComputePassDesc & {
    dispatch = ComputeDispatchDesc::dispatch1D(localSize, elementCount);
    return *this;
  }

  auto dispatch2D(uint32_t localSizeX, uint32_t elementCountX,
                  uint32_t localSizeY, uint32_t elementCountY)
      -> ComputePassDesc & {
    dispatch = ComputeDispatchDesc::dispatch2D(localSizeX, elementCountX,
                                               localSizeY, elementCountY);
    return *this;
  }

  auto dispatch3D(uint32_t localSizeX, uint32_t elementCountX,
                  uint32_t localSizeY, uint32_t elementCountY,
                  uint32_t localSizeZ, uint32_t elementCountZ)
      -> ComputePassDesc & {
    dispatch = ComputeDispatchDesc::dispatch3D(localSizeX, elementCountX,
                                               localSizeY, elementCountY,
                                               localSizeZ, elementCountZ);
    return *this;
  }

private:
  auto descriptorWrite(uint32_t setIndex)
      -> pipeline::DescriptorSetWrite & {
    for (auto &write : descriptorWrites) {
      if (write.setIndex == setIndex) {
        return write;
      }
    }

    descriptorWrites.push_back(
        pipeline::DescriptorSetWrite::forSet(setIndex));
    return descriptorWrites.back();
  }
};

class ComputePass final : public Pass {
public:
  explicit ComputePass(ComputeExecutor &executor, const core::Device &device);
  ~ComputePass() override;

  ComputePass(const ComputePass &) = delete;
  auto operator=(const ComputePass &) -> ComputePass & = delete;

  void create() override;
  void destroy() noexcept override;
  void update(const ComputePassDesc &desc);
  void record() override;

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
