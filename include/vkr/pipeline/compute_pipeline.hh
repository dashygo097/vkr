#pragma once

#include "vkr/core/device.hh"
#include "vkr/resource/shader/module.hh"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace vkr::pipe {

struct ComputePipelineLayoutDesc {
  std::vector<VkDescriptorSetLayout> setLayouts{};
  std::vector<VkPushConstantRange> pushConstants{};

  auto pushConstant(VkPushConstantRange range) -> ComputePipelineLayoutDesc & {
    pushConstants.push_back(range);
    return *this;
  }

  auto pushConstant(VkShaderStageFlags stageFlags, uint32_t offset,
                    uint32_t size) -> ComputePipelineLayoutDesc & {
    pushConstants.push_back({stageFlags, offset, size});
    return *this;
  }

  auto pushConstantRanges(std::vector<VkPushConstantRange> ranges)
      -> ComputePipelineLayoutDesc & {
    pushConstants = std::move(ranges);
    return *this;
  }

  auto clearPushConstants() noexcept -> ComputePipelineLayoutDesc & {
    pushConstants.clear();
    return *this;
  }
};

struct ComputePipelineDesc {
  std::string name{};
  res::ShaderModuleDesc shader{};
  std::string entryPoint{"main"};
  ComputePipelineLayoutDesc layout{};

  auto pushConstant(VkPushConstantRange range) -> ComputePipelineDesc & {
    layout.pushConstant(range);
    return *this;
  }

  auto pushConstant(VkShaderStageFlags stageFlags, uint32_t offset,
                    uint32_t size) -> ComputePipelineDesc & {
    layout.pushConstant(stageFlags, offset, size);
    return *this;
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return !name.empty() && !entryPoint.empty() && shader.isValid();
  }
};

class ComputePipeline {
public:
  class Version {
  public:
    explicit Version(const core::Device &device) : device_(device) {}
    ~Version();

    Version(const Version &) = delete;
    auto operator=(const Version &) -> Version & = delete;

    [[nodiscard]] auto pipeline() const noexcept -> VkPipeline {
      return pipeline_;
    }
    [[nodiscard]] auto layout() const noexcept -> VkPipelineLayout {
      return layout_;
    }

  private:
    friend class ComputePipeline;
    const core::Device &device_;
    VkPipeline pipeline_{VK_NULL_HANDLE};
    VkPipelineLayout layout_{VK_NULL_HANDLE};
  };

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
      -> const std::unique_ptr<res::ShaderModule> & {
    return shader_module_;
  }

  [[nodiscard]] auto layout() const noexcept -> VkPipelineLayout {
    return version_ ? version_->layout() : VK_NULL_HANDLE;
  }

  [[nodiscard]] auto pipeline() const noexcept -> VkPipeline {
    return version_ ? version_->pipeline() : VK_NULL_HANDLE;
  }

  [[nodiscard]] auto version() const noexcept
      -> std::shared_ptr<const Version> {
    return version_;
  }

  [[nodiscard]] auto bindPoint() const noexcept -> VkPipelineBindPoint {
    return VK_PIPELINE_BIND_POINT_COMPUTE;
  }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return pipeline() != VK_NULL_HANDLE;
  }

  [[nodiscard]] auto revision() const noexcept -> uint64_t { return revision_; }

private:
  // dependencies
  const core::Device &device_;

  // components
  ComputePipelineDesc desc_{};
  std::unique_ptr<res::ShaderModule> shader_module_{};
  std::shared_ptr<const Version> version_{};

  // states
  uint64_t revision_{0};
};

} // namespace vkr::pipe
