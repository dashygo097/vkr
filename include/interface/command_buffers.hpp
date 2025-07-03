#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "ctx.hpp"
#include "impl/vertex.hpp"

const int MAX_FRAMES_IN_FLIGHT = 2;

class CommandBuffers {
public:
  CommandBuffers(VkDevice device, VkCommandPool commandPool);
  CommandBuffers(const VulkanContext &ctx);
  ~CommandBuffers();

  std::vector<VkCommandBuffer> getVkCommandBuffers() const {
    return commandBuffers;
  }
  std::vector<VkCommandBuffer> &getVkCommandBuffersRef() {
    return commandBuffers;
  }

private:
  // dependencies
  VkDevice device;
  VkCommandPool commandPool;

  // components
  std::vector<VkCommandBuffer> commandBuffers{};
};

void recordCommandBuffer(uint32_t imageIndex, uint32_t currentFrame,
                         std::vector<Vertex> vertices,
                         std::vector<uint16_t> indices, VkBuffer indexBuffer,
                         VkBuffer vertexBuffer, VkCommandBuffer commandBuffer,
                         VkRenderPass renderPass,
                         VkPipelineLayout pipelineLayout,
                         std::vector<VkDescriptorSet> descriptorSets,
                         std::vector<VkFramebuffer> swapchainFrameBuffers,
                         VkExtent2D swapchainExtent,
                         VkPipeline graphicsPipeline);
