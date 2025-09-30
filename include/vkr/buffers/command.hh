#pragma once

#include "../components/ui/ui.hh"
#include "../ctx.hh"
#include "./index.hh"
#include "./vertex.hh"

const int MAX_FRAMES_IN_FLIGHT = 2;

namespace vkr {
class CommandBuffers {
public:
  CommandBuffers(VkDevice device, VkCommandPool commandPool);
  CommandBuffers(const VulkanContext &ctx);
  ~CommandBuffers();

  CommandBuffers(const CommandBuffers &) = delete;
  CommandBuffers &operator=(const CommandBuffers &) = delete;

  [[nodiscard]] std::vector<VkCommandBuffer> commandBuffers() const noexcept {
    return _commandBuffers;
  }
  [[nodiscard]] std::vector<VkCommandBuffer> &commandBuffersRef() noexcept {
    return _commandBuffers;
  }

  [[nodiscard]] VkCommandBuffer commandBuffer(uint32_t index) const noexcept {
    return _commandBuffers[index];
  }
  [[nodiscard]] VkCommandBuffer &commandBufferRef(uint32_t index) noexcept {
    return _commandBuffers[index];
  }

  void record(uint32_t imageIndex, uint32_t currentFrame,
              VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
              const std::vector<VkDescriptorSet> &descriptorSets,
              const std::vector<VkFramebuffer> &framebuffers, VkExtent2D extent,
              VkPipeline graphicsPipeline,
              const std::vector<std::unique_ptr<VertexBuffer>> &vertexBuffers,
              const std::vector<std::unique_ptr<IndexBuffer>> &indexBuffers,
              UI &ui);

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};
  VkCommandPool commandPool{VK_NULL_HANDLE};

  // components
  std::vector<VkCommandBuffer> _commandBuffers{};

  void beginRecording(uint16_t index);
  void beginRenderPass(uint16_t index, VkRenderPass renderPass,
                       VkFramebuffer framebuffer, VkExtent2D extent2d);
  void setViewportAndScissor(uint16_t index, VkExtent2D extent);

  void drawIndexed(uint16_t index, VkPipelineLayout pipelineLayout,
                   VkDescriptorSet descriptorSet,
                   const std::unique_ptr<VertexBuffer> &vertexBuffer,
                   const std::unique_ptr<IndexBuffer> &indexBuffer);

  void endRenderPass(uint16_t index);
  void endRecording(uint16_t index);
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
