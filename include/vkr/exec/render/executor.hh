#pragma once

#include "vkr/core/command/buffers.hh"
#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/swapchain.hh"
#include "vkr/core/sync/fence.hh"
#include "vkr/core/sync/semaphore.hh"
#include "vkr/exec/profiler.hh"
#include "vkr/exec/render/frame_buffer_set.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/scene/scene.hh"
#include "vkr/ui/ui.hh"
#include <functional>
#include <optional>
#include <string_view>
#include <vector>

namespace vkr::exec {

class RenderExecutor {
public:
  explicit RenderExecutor(const core::Device &device,
                          const core::Swapchain &swapchain,
                          const core::CommandPool &commandPool,
                          scene::Scene &scene,
                          core::CommandBuffers &commandBuffers);
  ~RenderExecutor() = default;

  RenderExecutor(const RenderExecutor &) = delete;
  auto operator=(const RenderExecutor &) -> RenderExecutor & = delete;

  auto beginFrame() -> bool;
  void submitFrame();
  void presentFrame();
  void endFrame();
  void setProfiler(Profiler &profiler) noexcept;
  void clearProfiler() noexcept;

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

  [[nodiscard]] auto framesInFlight() const noexcept -> uint32_t;

  void beginPass(const FramebufferSet &framebufferSet,
                 const std::vector<VkClearValue> &clearValues,
                 uint32_t framebufferIndex = 0,
                 VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
  void endPass();

  void bindPipeline(const pipeline::GraphicsPipeline &pipeline,
                    const pipeline::DescriptorSets &sets);
  void setViewportAndScissor(VkExtent2D extent);

  void drawIndexed(const scene::IVertexBuffer &vertexBuffer,
                   const scene::IndexBuffer &indexBuffer);
  void drawGeometry();
  void drawFullscreenTriangle();
  void drawUI(ui::UI &ui);
  void beginProfileScope(std::string_view name);
  void endProfileScope();

private:
  // dependencies
  const core::Device &device_;
  const core::Swapchain &swapchain_;
  const core::CommandPool &command_pool_;
  scene::Scene &scene_;
  core::CommandBuffers &command_buffers_;
  std::optional<std::reference_wrapper<Profiler>> profiler_{};

  // components
  std::vector<core::Semaphore> image_available_{};
  std::vector<core::Semaphore> render_finished_{};
  std::vector<core::Fence> in_flight_{};

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
  void ensureFrameActive(std::string_view op) const;
  void ensureFrameInactive(std::string_view op) const;

  auto acquireNextImage(uint32_t &imageIndex) -> bool;
  void submitCommandBuffer();
  void present(uint32_t imageIndex);
};

} // namespace vkr::exec
