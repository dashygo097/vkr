#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/render_pass.hh"
#include <cstddef>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::exec {

struct FramebuffersDesc {
  uint32_t width{};
  uint32_t height{};
  uint32_t layers{1};
  std::vector<std::vector<VkImageView>> attachments{};

  auto extent(uint32_t framebufferWidth, uint32_t framebufferHeight) noexcept
      -> FramebuffersDesc & {
    width = framebufferWidth;
    height = framebufferHeight;
    return *this;
  }

  auto layerCount(uint32_t count) noexcept -> FramebuffersDesc & {
    layers = count;
    return *this;
  }

  auto attachmentViews(std::vector<VkImageView> views) -> FramebuffersDesc & {
    attachments.push_back(std::move(views));
    return *this;
  }

  auto attachmentViews(std::vector<std::vector<VkImageView>> views)
      -> FramebuffersDesc & {
    attachments = std::move(views);
    return *this;
  }

  [[nodiscard]] static auto single(uint32_t width, uint32_t height,
                                   std::vector<VkImageView> views)
      -> FramebuffersDesc {
    FramebuffersDesc desc{};
    return desc.extent(width, height)
        .layerCount(1)
        .attachmentViews(std::move(views));
  }
};

class Framebuffers {
public:
  explicit Framebuffers(const core::Device &device,
                        const pipeline::RenderPass &renderPass);

  ~Framebuffers();

  Framebuffers(const Framebuffers &) = delete;
  auto operator=(const Framebuffers &) -> Framebuffers & = delete;

  void create();
  void destroy();
  void update(const FramebuffersDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const FramebuffersDesc & {
    return desc_;
  }

  [[nodiscard]] auto buffers() const noexcept
      -> const std::vector<VkFramebuffer> & {
    return vk_framebuffers_;
  }

  [[nodiscard]] auto buffer(size_t index) const -> VkFramebuffer {
    return vk_framebuffers_.at(index);
  }

  [[nodiscard]] auto extent() const noexcept -> VkExtent2D {
    return {desc_.width, desc_.height};
  }

private:
  // dependencies
  const core::Device &device_;
  const pipeline::RenderPass &render_pass_;

  // components
  FramebuffersDesc desc_{};
  std::vector<VkFramebuffer> vk_framebuffers_{};
};

} // namespace vkr::exec
