#pragma once

#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/attachments/color.hh"
#include "vkr/resource/attachments/depth.hh"

namespace vkr::resource {

struct OffscreenTargetDesc {
  ColorAttachmentDesc color{};
  std::optional<DepthAttachmentDesc> depth{};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    if (color.format == VK_FORMAT_UNDEFINED || color.width == 0 ||
        color.height == 0) {
      return false;
    }

    if (!depth) {
      return true;
    }

    if (depth->format == VK_FORMAT_UNDEFINED || depth->width == 0 ||
        depth->height == 0) {
      return false;
    }

    if (depth->width != color.width || depth->height != color.height) {
      return false;
    }

    return true;
  }
};

class OffscreenTarget {
public:
  OffscreenTarget(const core::Device &device,
                  const core::CommandPool &commandPool);

  ~OffscreenTarget();

  OffscreenTarget(const OffscreenTarget &) = delete;
  auto operator=(const OffscreenTarget &) -> OffscreenTarget & = delete;

  [[nodiscard]] auto desc() const noexcept -> const OffscreenTargetDesc & {
    return desc_;
  }

  void create();
  void destory();
  void update(const OffscreenTargetDesc &desc);

  [[nodiscard]] auto color() noexcept -> ColorAttachment & { return *color_; }

  [[nodiscard]] auto color() const noexcept -> const ColorAttachment & {
    return *color_;
  }

  [[nodiscard]] auto depth() noexcept -> DepthAttachment * {
    return depth_.get();
  }

  [[nodiscard]] auto depth() const noexcept -> const DepthAttachment * {
    return depth_.get();
  }

  [[nodiscard]] auto hasDepth() const noexcept -> bool {
    return depth_ != nullptr;
  }

  [[nodiscard]] auto extent2D() const noexcept -> VkExtent2D {
    return {desc_.color.width, desc_.color.height};
  }

  [[nodiscard]] auto attachmentViews() const -> std::vector<VkImageView>;

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  OffscreenTargetDesc desc_{};
  std::unique_ptr<ColorAttachment> color_;
  std::unique_ptr<DepthAttachment> depth_;
};

} // namespace vkr::resource
