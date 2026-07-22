#pragma once

#include "vkr/core/command/buffers.hh"
#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/exec/render/attachments/frame_buffer.hh"
#include "vkr/exec/render/sync/frame_sync.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/scene/scene.hh"
#include "vkr/ui/ui.hh"

namespace vkr::exec {

struct RenderPassBeginDesc {
  uint32_t framebufferIndex{0};
  VkRect2D renderArea{};
  std::vector<VkClearValue> clearValues{};
  VkSubpassContents contents{VK_SUBPASS_CONTENTS_INLINE};
};

class Executor {
public:
  explicit Executor(const core::Device &device,
                    const core::Swapchain &swapchain,
                    const core::CommandPool &commandPool, FrameSync &frameSync,
                    scene::Scene &scene);
  ~Executor() = default;

  Executor(const Executor &) = delete;
  auto operator=(const Executor &) -> Executor & = delete;

  auto beginFrame() -> bool;
  void submitFrame();
  void presentFrame();
  void endFrame();

  [[nodiscard]] auto swapchainOutOfDate() const noexcept -> bool {
    return swapchain_out_of_date_;
  }

  [[nodiscard]] auto consumeSwapchainOutOfDate() noexcept -> bool {
    const bool outOfDate = swapchain_out_of_date_;
    swapchain_out_of_date_ = false;
    return outOfDate;
  }

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

  void beginPass(const FramebufferSet &framebufferSet,
                 const pipeline::RenderPass &renderPass,
                 const RenderPassBeginDesc &desc);
  void endPass();

  void bindPipeline(VkPipeline pipeline, VkPipelineLayout pipelineLayout,
                    const std::vector<VkDescriptorSet> &descriptorSets);
  void setViewportAndScissor(VkExtent2D extent);

  void drawIndexed(const scene::IVertexBuffer &vertexBuffer,
                   const scene::IndexBuffer &indexBuffer);
  void drawGeometry();
  void drawFullscreenTriangle();
  void drawUI(ui::UI &ui);

private:
  // dependencies
  const core::Device &device_;
  const core::Swapchain &swapchain_;
  const core::CommandPool &command_pool_;
  FrameSync &frame_sync_;
  scene::Scene &scene_;

  // components
  std::unique_ptr<core::CommandBuffers> command_buffers_{};

  // state
  uint32_t current_frame_{0};
  uint32_t image_index_{0};
  uint32_t frame_index_{0};
  VkCommandBuffer command_buffer_{VK_NULL_HANDLE};
  bool frame_active_{false};
  bool frame_submitted_{false};
  bool frame_presented_{false};
  bool swapchain_out_of_date_{false};

  // helpers
  void ensureFrameActive(const char *op) const;
  void ensureFrameInactive(const char *op) const;

  auto acquireNextImage(uint32_t &imageIndex) -> bool;
  void submitCommandBuffer();
  void present(uint32_t imageIndex);
};

} // namespace vkr::exec
