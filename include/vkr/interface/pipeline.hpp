#pragma once

#include "../ctx.hpp"

namespace vkr {

class GraphicsPipeline {
public:
  GraphicsPipeline(VkDevice device, VkRenderPass renderPass,
                   VkDescriptorSetLayout descriptorSetLayout,
                   const std::string &vertShaderPath,
                   const std::string &fragShaderPath);
  GraphicsPipeline(const VulkanContext &ctx);
  ~GraphicsPipeline();

  GraphicsPipeline(const GraphicsPipeline &) = delete;
  GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

  [[nodiscard]] VkPipelineLayout pipelineLayout() const noexcept {
    return _pipelineLayout;
  }
  [[nodiscard]] VkPipeline pipeline() const noexcept {
    return _graphicsPipeline;
  }

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};
  VkRenderPass renderPass{VK_NULL_HANDLE};
  VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};

  // componets
  VkPipelineLayout _pipelineLayout{VK_NULL_HANDLE};
  VkPipeline _graphicsPipeline{VK_NULL_HANDLE};
};
} // namespace vkr
