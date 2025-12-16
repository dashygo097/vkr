#pragma once

#include "../components/ui/ui.hh"
#include "../resources/buffers/index_buffer.hh"
#include "../resources/buffers/vertex_buffer.hh"
#include "./command_pool.hh"
#include "./device.hh"

const int MAX_FRAMES_IN_FLIGHT = 2;

namespace vkr {
class CommandBuffers {
public:
  explicit CommandBuffers(const Device &device, const CommandPool &commandPool);
  ~CommandBuffers();

  CommandBuffers(const CommandBuffers &) = delete;
  CommandBuffers &operator=(const CommandBuffers &) = delete;

  // FIXME: ref return
  [[nodiscard]] std::vector<VkCommandBuffer> &commandBuffers() noexcept {
    return _commandBuffers;
  }
  [[nodiscard]] VkCommandBuffer &commandBuffer(uint32_t index) noexcept {
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
  const Device &device;
  const CommandPool &commandPool;

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
