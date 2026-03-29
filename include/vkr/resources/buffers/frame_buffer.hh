#pragma once

#include "../../core/device.hh"
#include "../../core/swapchain.hh"
#include "../../pipeline/render_pass.hh"
#include "../depth_resources.hh"

namespace vkr::resource {

class Framebuffers {
public:
  explicit Framebuffers(const core::Device &device,
                        const core::Swapchain &swapchain,
                        const DepthResources &depthResources,
                        const pipeline::RenderPass &renderPass);
  ~Framebuffers();

  Framebuffers(const Framebuffers &) = delete;
  auto operator=(const Framebuffers &) -> Framebuffers & = delete;

  void create();
  void destroy();

  [[nodiscard]] auto framebuffers() const noexcept
      -> std::vector<VkFramebuffer> {
    return vk_framebuffers_;
  }

private:
  // dependencies
  const core::Device &device_;
  const core::Swapchain &swapchain_;
  const DepthResources &depth_resources_;
  const pipeline::RenderPass &render_pass_;

  // components
  std::vector<VkFramebuffer> vk_framebuffers_{};
  bool frame_buffer_resized_{false};
};
} // namespace vkr::resource
