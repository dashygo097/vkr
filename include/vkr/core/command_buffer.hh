#pragma once

#include "../buffers/index.hh"
#include "../buffers/vertex.hh"
#include "../components/ui/ui.hh"
#include "../ctx.hh"

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
              const std::vector<std::shared_ptr<VertexBuffer>> &vertexBuffers,
              const std::vector<std::shared_ptr<IndexBuffer>> &indexBuffers,
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
                   const std::shared_ptr<VertexBuffer> &vertexBuffer,
                   const std::shared_ptr<IndexBuffer> &indexBuffer);

  void endRenderPass(uint16_t index);
  void endRecording(uint16_t index);
};

} // namespace vkr
