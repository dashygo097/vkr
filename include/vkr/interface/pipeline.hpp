#pragma once

#include <string>
#include <vulkan/vulkan.h>

#include "../ctx.hpp"

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

  VkPipelineLayout getVkPipelineLayout() const { return pipelineLayout; }
  VkPipeline getVkPipeline() const { return graphicsPipeline; }

private:
  // dependencies
  VkDevice device;
  VkRenderPass renderPass;
  VkDescriptorSetLayout descriptorSetLayout;

  // componets
  VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  VkPipeline graphicsPipeline{VK_NULL_HANDLE};
};
