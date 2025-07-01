#pragma once

#include <string>
#include <vulkan/vulkan.h>

#include "ctx.hpp"

class GraphicsPipeline {
public:
  GraphicsPipeline(VkDevice device, VkRenderPass renderPass,
                   const std::string &vertShaderPath,
                   const std::string &fragShaderPath);
  GraphicsPipeline(const VulkanContext &ctx);
  ~GraphicsPipeline();

  VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
  VkPipeline getPipeline() const { return graphicsPipeline; }

private:
  // dependencies
  VkDevice device;
  VkRenderPass renderPass;

  // componets
  VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  VkPipeline graphicsPipeline{VK_NULL_HANDLE};
};
