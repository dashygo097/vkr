#pragma once

#include "../../core/device.hh"
#include "../../core/swapchain.hh"
#include "../../pipeline/render_pass.hh"

namespace vkr {

class Framebuffers {
public:
  explicit Framebuffers(const Device &device, const RenderPass &renderPass,
                        const Swapchain &swapchain);
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
  const Device &device;
  const RenderPass &renderPass;
  const Swapchain &swapchain;

  // components
  std::vector<VkFramebuffer> _framebuffers{};
  bool frameBufferResized{false};
};
} // namespace vkr
