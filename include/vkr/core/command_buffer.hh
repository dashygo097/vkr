#pragma once

#include "../resources/buffers/index_buffer.hh"
#include "../resources/buffers/vertex_buffer.hh"
#include "../ui/ui.hh"
#include "./command_pool.hh"
#include "./device.hh"

const int MAX_FRAMES_IN_FLIGHT = 2;

namespace vkr::core {
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
  // FIXME: ref return
  [[nodiscard]] VkCommandBuffer &commandBuffer(uint32_t index) noexcept {
    return _commandBuffers[index];
  }

  void record(
      uint32_t imageIndex, uint32_t currentFrame, VkRenderPass renderPass,
      VkPipelineLayout pipelineLayout,
      const std::vector<VkDescriptorSet> &descriptorSets,
      const std::vector<VkFramebuffer> &framebuffers, VkExtent2D extent,
      VkPipeline graphicsPipeline,
      const std::vector<std::shared_ptr<resource::IVertexBuffer>>
          &vertexBuffers,
      const std::vector<std::shared_ptr<resource::IndexBuffer>> &indexBuffers,
      ui::UI &ui);

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

  void endRenderPass(uint16_t index);
  void endRecording(uint16_t index);
};

} // namespace vkr::core
