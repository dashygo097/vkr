#pragma once

#include "../../core/device.hh"
#include "../../core/swapchain.hh"
#include "../../pipeline/render_pass.hh"

namespace vkr::resource {

class Framebuffers {
public:
  explicit Framebuffers(const core::Device &device,
                        const core::Swapchain &swapchain,
                        const pipeline::RenderPass &renderPass);
  ~Framebuffers();

  Framebuffers(const Framebuffers &) = delete;
  Framebuffers &operator=(const Framebuffers &) = delete;

  void create();
  void destroy();

  [[nodiscard]] std::vector<VkFramebuffer> framebuffers() const noexcept {
    return _framebuffers;
  }

private:
  // dependencies
  const core::Device &device;
  const core::Swapchain &swapchain;
  const pipeline::RenderPass &renderPass;

  // components
  std::vector<VkFramebuffer> _framebuffers{};
  bool frameBufferResized{false};
};
} // namespace vkr::resource
