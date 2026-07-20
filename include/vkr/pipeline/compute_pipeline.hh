#pragma once

#include "vkr/core/device.hh"
#include "vkr/resource/shader/module.hh"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace vkr::pipeline {

struct ComputePipelineLayoutDesc {
  std::vector<VkDescriptorSetLayout> setLayouts{};
  std::vector<VkPushConstantRange> pushConstants{};
};

struct ComputePipelineDesc {
  std::string name{};
  resource::ShaderModuleDesc shader{};
  std::string entryPoint{"main"};
  ComputePipelineLayoutDesc layout{};

  VkPipeline basePipeline{VK_NULL_HANDLE};
  int32_t basePipelineIndex{-1};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return !name.empty() && !entryPoint.empty() && shader.isValid();
  }
};

class ComputePipeline {
public:
  explicit ComputePipeline(const core::Device &device);
  ~ComputePipeline();

  ComputePipeline(const ComputePipeline &) = delete;
  auto operator=(const ComputePipeline &) -> ComputePipeline & = delete;

  void destroy();
  auto update(const ComputePipelineDesc &desc) -> bool;

  [[nodiscard]] auto desc() const noexcept -> const ComputePipelineDesc & {
    return desc_;
  }

  [[nodiscard]] auto shaderModule() const noexcept
      -> const std::unique_ptr<resource::ShaderModule> & {
    return shader_module_;
  }

  [[nodiscard]] auto layout() const noexcept -> VkPipelineLayout {
    return vk_pipeline_layout_;
  }

  [[nodiscard]] auto pipeline() const noexcept -> VkPipeline {
    return vk_compute_pipeline_;
  }

  [[nodiscard]] auto bindPoint() const noexcept -> VkPipelineBindPoint {
    return VK_PIPELINE_BIND_POINT_COMPUTE;
  }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return vk_compute_pipeline_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] auto revision() const noexcept -> uint64_t { return revision_; }

private:
  struct RetiredPipeline {
    VkPipeline pipeline{VK_NULL_HANDLE};
    VkPipelineLayout layout{VK_NULL_HANDLE};
  };

  const core::Device &device_;

  ComputePipelineDesc desc_{};
  std::unique_ptr<resource::ShaderModule> shader_module_{};
  VkPipelineLayout vk_pipeline_layout_{VK_NULL_HANDLE};
  VkPipeline vk_compute_pipeline_{VK_NULL_HANDLE};
  std::vector<RetiredPipeline> retired_pipelines_{};
  uint64_t revision_{0};
};

} // namespace vkr::pipeline
