#pragma once

#include "../core/command_buffer.hh"
#include "../core/command_pool.hh"
#include "../core/device.hh"
#include "../core/swapchain.hh"
#include "../core/sync_objects.hh"
#include "../pipeline/render_pass.hh"
#include "../resources/buffers/frame_buffer.hh"
#include "../resources/manager.hh"
#include "../resources/offscreen_target.hh"
#include "../ui/ui.hh"

namespace vkr::render {

struct FrameData {
  uint32_t imageIndex{0};
  uint32_t frameIndex{0};
  VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
};

struct RenderPassBeginDesc {
  const pipeline::RenderPass *renderPass{nullptr};
  const resource::FramebufferSet *framebufferSet{nullptr};

  uint32_t framebufferIndex{0};

  VkRect2D renderArea{};
  std::vector<VkClearValue> clearValues{};

  VkSubpassContents contents{VK_SUBPASS_CONTENTS_INLINE};
};

class Renderer {
public:
  using SwapchainRecreateCallback = std::function<void()>;

  explicit Renderer(core::Device &device, core::Swapchain &swapchain,
                    const core::CommandPool &commandPool,
                    const core::SyncObjects &syncObjects,
                    resource::ResourceManager &resourceManager,
                    const pipeline::RenderPass &renderPass, ui::UI &ui);
  ~Renderer();

  Renderer(const Renderer &) = delete;
  auto operator=(const Renderer &) -> Renderer & = delete;

  auto beginFrame(FrameData &outFrameData) -> bool;
  void endFrame(const FrameData &frameData);

  // Generic pass API
  void beginPass(const FrameData &frameData, const RenderPassBeginDesc &desc);
  void endPass(const FrameData &frameData);

  // Swapchain pass
  void beginRenderPass(const FrameData &frameData);
  void endRenderPass(const FrameData &frameData);

  // Offscreen pass
  void beginOffscreenPass(const FrameData &frameData,
                          const pipeline::RenderPass &renderPass,
                          const resource::FramebufferSet &framebufferSet,
                          const resource::OffscreenTarget &target);
  void endOffscreenPass(const FrameData &frameData);

  void bindPipeline(const FrameData &frameData, VkPipeline pipeline,
                    VkPipelineLayout pipelineLayout,
                    const std::vector<VkDescriptorSet> &descriptorSets);

  void setViewportAndScissor(const FrameData &frameData, VkExtent2D extent);
  void setViewportAndScissor(const FrameData &frameData);

  void setOffscreenViewportAndScissor(const FrameData &frameData,
                                      const resource::OffscreenTarget &target);

  void drawGeometry(const FrameData &frameData);
  void drawUI(const FrameData &frameData);

  void recreateSwapchain();

  void setSwapchainRecreateCallback(SwapchainRecreateCallback callback) {
    swapchain_recreate_callback_ = std::move(callback);
  }

  [[nodiscard]] auto currentFrameIndex() const noexcept -> uint32_t {
    return current_frame_;
  }

  void setFramebufferResized(bool value) noexcept {
    framebuffer_resized_ = value;
  }

private:
  // dependencies
  core::Device &device_;
  core::Swapchain &swapchain_;
  const core::CommandPool &command_pool_;
  const core::SyncObjects &sync_objects_;
  resource::ResourceManager &resource_manager_;
  const pipeline::RenderPass &render_pass_;
  ui::UI &ui_;

  // components
  std::unique_ptr<core::CommandBuffers> command_buffers_{};

  // state
  uint32_t current_frame_{0};
  bool framebuffer_resized_{false};
  SwapchainRecreateCallback swapchain_recreate_callback_{};

  void waitForFence(uint32_t frameIndex);
  void resetFence(uint32_t frameIndex);
  auto acquireNextImage(uint32_t &imageIndex) -> bool;
  void submitCommandBuffer(const FrameData &frameData);
  void present(uint32_t imageIndex);
};

} // namespace vkr::render
