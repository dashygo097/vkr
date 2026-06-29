#pragma once

#include "vkr/core/command/command_buffer.hh"
#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/swapchain.hh"
#include "vkr/core/sync/sync_objects.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/resource/buffers/frame_buffer.hh"
#include "vkr/resource/manager.hh"
#include "vkr/resource/offscreen_target.hh"
#include "vkr/ui/ui.hh"

namespace vkr::render {

struct FrameData {
  uint32_t imageIndex{0};
  uint32_t frameIndex{0};
  VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
};

struct RenderPassBeginDesc {
  uint32_t framebufferIndex{0};

  VkRect2D renderArea{};
  std::vector<VkClearValue> clearValues{};

  VkSubpassContents contents{VK_SUBPASS_CONTENTS_INLINE};
};

class Renderer {
public:
  explicit Renderer(core::Device &device, core::Swapchain &swapchain,
                    const core::CommandPool &commandPool,
                    core::SyncObjects &syncObjects,
                    resource::ResourceManager &resourceManager, ui::UI &ui);
  ~Renderer();

  Renderer(const Renderer &) = delete;
  auto operator=(const Renderer &) -> Renderer & = delete;

  auto beginFrame(FrameData &outFrameData) -> bool;
  void endFrame(const FrameData &frameData);

  // Generic pass API
  void beginPass(const FrameData &frameData,
                 const resource::FramebufferSet &framebufferSet,
                 const pipeline::RenderPass &renderPass,
                 const RenderPassBeginDesc &desc);
  void endPass(const FrameData &frameData);

  // Swapchain pass
  void beginSwapchainPass(const FrameData &frameData,
                          const resource::FramebufferSet &framebufferSet,
                          const pipeline::RenderPass &renderPass);

  // Offscreen pass
  void beginOffscreenPass(const FrameData &frameData,
                          const resource::FramebufferSet &framebufferSet,
                          const pipeline::RenderPass &renderPass,
                          const resource::OffscreenTarget &target);

  void bindPipeline(const FrameData &frameData, VkPipeline pipeline,
                    VkPipelineLayout pipelineLayout,
                    const std::vector<VkDescriptorSet> &descriptorSets);

  void setViewportAndScissor(const FrameData &frameData, VkExtent2D extent);
  void setViewportAndScissor(const FrameData &frameData);

  void setOffscreenViewportAndScissor(const FrameData &frameData,
                                      const resource::OffscreenTarget &target);

  void drawGeometry(const FrameData &frameData);
  void drawUI(const FrameData &frameData);

  [[nodiscard]] auto currentFrameIndex() const noexcept -> uint32_t {
    return current_frame_;
  }

private:
  // dependencies
  core::Device &device_;
  core::Swapchain &swapchain_;
  const core::CommandPool &command_pool_;
  core::SyncObjects &sync_objects_;
  resource::ResourceManager &resource_manager_;
  ui::UI &ui_;

  // components
  std::unique_ptr<core::CommandBuffers> command_buffers_{};

  // state
  uint32_t current_frame_{0};

  void waitForFence(uint32_t frameIndex);
  void resetFence(uint32_t frameIndex);
  auto acquireNextImage(uint32_t &imageIndex) -> bool;
  void submitCommandBuffer(const FrameData &frameData);
  void present(uint32_t imageIndex);
};

} // namespace vkr::render
