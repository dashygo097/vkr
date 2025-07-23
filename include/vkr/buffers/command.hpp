#pragma once

#include "../ctx.hpp"
#include "../ui/ui.hpp"
#include "./index.hpp"
#include "./vertex.hpp"

const int MAX_FRAMES_IN_FLIGHT = 2;

namespace vkr {
class CommandBuffers {
public:
  CommandBuffers(VkDevice device, VkCommandPool commandPool);
  CommandBuffers(const VulkanContext &ctx);
  ~CommandBuffers();

  CommandBuffers(const CommandBuffers &) = delete;
  CommandBuffers &operator=(const CommandBuffers &) = delete;

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

void recordCommandBuffer(
    uint32_t imageIndex, uint32_t currentFrame, VkCommandBuffer commandBuffer,
    VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
    std::vector<VkDescriptorSet> descriptorSets,
    std::vector<VkFramebuffer> swapchainFrameBuffers,
    VkExtent2D swapchainExtent, VkPipeline graphicsPipeline,
    const std::vector<std::unique_ptr<VertexBuffer>> &vertexBuffers,
    const std::vector<std::unique_ptr<IndexBuffer>> &indexBuffers, UI &ui);

VkCommandBuffer beginSingleTimeCommands(VkDevice device,
                                        VkCommandPool commandPool);

void endSingleTimeCommands(VkDevice device, VkQueue queue,
                           VkCommandPool commandPool,
                           VkCommandBuffer commandBuffer);

} // namespace vkr
