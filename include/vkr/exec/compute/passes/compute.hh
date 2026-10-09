#pragma once

#include "vkr/core/device.hh"
#include "vkr/exec/compute/executor.hh"
#include "vkr/exec/pass.hh"
#include "vkr/pipeline/compute_pipeline.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
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
  std::vector<pipeline::DescriptorSetLayoutDesc> descriptorLayouts{};
  pipeline::ComputePipelineDesc pipeline{};
  ComputeDispatchDesc dispatch{};

  auto descriptor(uint32_t setIndex, pipeline::DescriptorBinding binding)
      -> ComputePassDesc & {
    if (setIndex >= descriptorLayouts.size()) {
      descriptorLayouts.resize(static_cast<size_t>(setIndex) + 1);
    }
    descriptorLayouts[setIndex].bindings.push_back(std::move(binding));
    return *this;
  }

  auto storage(uint32_t setIndex, uint32_t binding) -> ComputePassDesc & {
    return descriptor(setIndex,
                      {.layout = {binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                  VK_SHADER_STAGE_COMPUTE_BIT}});
  }

  auto uniform(uint32_t setIndex, uint32_t binding) -> ComputePassDesc & {
    return descriptor(setIndex,
                      {.layout = {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                                  VK_SHADER_STAGE_COMPUTE_BIT}});
  }

  auto texture(uint32_t setIndex, uint32_t binding) -> ComputePassDesc & {
    return descriptor(
        setIndex,
        {.layout = {binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    VK_SHADER_STAGE_COMPUTE_BIT}});
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

  template <typename T>
  auto storage(uint32_t setIndex, uint32_t binding, T &buffer)
      -> ComputePass & {
    descriptor_sets_.at(setIndex).storage(binding, buffer);
    return *this;
  }

  template <typename T>
  auto uniform(uint32_t setIndex, uint32_t binding, T &buffer)
      -> ComputePass & {
    descriptor_sets_.at(setIndex).uniform(binding, buffer);
    return *this;
  }

  template <typename T>
  auto texture(uint32_t setIndex, uint32_t binding, T &texture)
      -> ComputePass & {
    descriptor_sets_.at(setIndex).texture(binding, texture);
    return *this;
  }

private:
  // dependencies
  ComputeExecutor &executor_;
  const core::Device &device_;

  // components
  ComputePassDesc desc_{};
  pipeline::DescriptorPool descriptor_pool_;
  std::vector<std::unique_ptr<pipeline::DescriptorSetLayout>>
      descriptor_layouts_{};
  std::vector<pipeline::DescriptorSet> descriptor_sets_{};
  std::unique_ptr<pipeline::ComputePipeline> pipeline_{};

  // helpers
  void validate(const ComputePassDesc &desc) const;
  void createDescriptors();
  void createPipeline();
};

} // namespace vkr::exec
