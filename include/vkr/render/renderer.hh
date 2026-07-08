#pragma once

#include "vkr/core/command/command_buffer.hh"
#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/sync/sync_objects.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/resource/attachments/frame_buffer.hh"
#include "vkr/resource/manager.hh"
#include "vkr/resource/targets/offscreen.hh"
#include "vkr/resource/targets/swapchain.hh"
#include "vkr/ui/ui.hh"

namespace vkr::render {

struct RenderPassBeginDesc {
  uint32_t framebufferIndex{0};
  VkRect2D renderArea{};
  std::vector<VkClearValue> clearValues{};
  VkSubpassContents contents{VK_SUBPASS_CONTENTS_INLINE};
};

class Renderer {
public:
  explicit Renderer(const core::Device &device,
                    const core::Swapchain &swapchain,
                    const core::CommandPool &commandPool,
                    core::SyncObjects &syncObjects,
                    resource::ResourceManager &resourceManager, ui::UI &ui);
  ~Renderer() = default;

  Renderer(const Renderer &) = delete;
  auto operator=(const Renderer &) -> Renderer & = delete;

  auto beginFrame() -> bool;
  void endFrame();

  [[nodiscard]] auto commandBuffer() const -> VkCommandBuffer {
    ensureFrameActive("commandBuffer");
    return command_buffer_;
  }

  [[nodiscard]] auto frameIndex() const noexcept -> uint32_t {
    return frame_index_;
  }

  [[nodiscard]] auto imageIndex() const noexcept -> uint32_t {
    return image_index_;
  }

  [[nodiscard]] auto currentFrameIndex() const noexcept -> uint32_t {
    return current_frame_;
  }

  void beginPass(const resource::FramebufferSet &framebufferSet,
                 const pipeline::RenderPass &renderPass,
                 const RenderPassBeginDesc &desc);
  void endPass();

  void beginSwapchainPass(const resource::FramebufferSet &framebufferSet,
                          const pipeline::RenderPass &renderPass,
                          const resource::SwapchainTarget &swapchainTarget);

  void beginOffscreenPass(const resource::FramebufferSet &framebufferSet,
                          const pipeline::RenderPass &renderPass,
                          const resource::OffscreenTarget &target);

  void bindPipeline(VkPipeline pipeline, VkPipelineLayout pipelineLayout,
                    const std::vector<VkDescriptorSet> &descriptorSets);

  void setViewportAndScissor(VkExtent2D extent);
  void setViewportAndScissor();

  void setOffscreenViewportAndScissor(const resource::OffscreenTarget &target);

  void drawGeometry();
  void drawUI();

private:
  // dependencies
  const core::Device &device_;
  const core::Swapchain &swapchain_;
  const core::CommandPool &command_pool_;
  core::SyncObjects &sync_objects_;
  resource::ResourceManager &resource_manager_;
  ui::UI &ui_;

  // components
  std::unique_ptr<core::CommandBuffers> command_buffers_{};

  // state
  uint32_t current_frame_{0};
  uint32_t image_index_{0};
  uint32_t frame_index_{0};
  VkCommandBuffer command_buffer_{VK_NULL_HANDLE};
  bool frame_active_{false};

  // helpers
  void ensureFrameActive(const char *op) const;
  void ensureFrameInactive(const char *op) const;

  auto acquireNextImage(uint32_t &imageIndex) -> bool;
  void submitCommandBuffer();
  void present(uint32_t imageIndex);
};

} // namespace vkr::render
