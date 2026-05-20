#pragma once

#include "../../core/device.hh"
#include "../../pipeline/render_pass.hh"
#include <cstddef>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::resource {

struct FramebufferDesc {
  VkExtent2D extent{};
  uint32_t layers{1};
  std::vector<std::vector<VkImageView>> attachments{};
};

class FramebufferSet {
public:
  explicit FramebufferSet(const core::Device &device,
                          const pipeline::RenderPass &renderPass);

  ~FramebufferSet();

  FramebufferSet(const FramebufferSet &) = delete;
  auto operator=(const FramebufferSet &) -> FramebufferSet & = delete;

  void create();
  void destroy();
  void update(const FramebufferDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const FramebufferDesc & {
    return desc_;
  }

  [[nodiscard]] auto buffers() const noexcept
      -> const std::vector<VkFramebuffer> & {
    return vk_framebuffers_;
  }

  [[nodiscard]] auto buffer(size_t index) const -> VkFramebuffer {
    return vk_framebuffers_.at(index);
  }

private:
  // dependencies
  const core::Device &device_;
  const pipeline::RenderPass &render_pass_;

  // components
  FramebufferDesc desc_{};
  std::vector<VkFramebuffer> vk_framebuffers_{};
};

} // namespace vkr::resource
