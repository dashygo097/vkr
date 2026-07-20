#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/exec/render/attachments/color.hh"
#include "vkr/exec/render/attachments/depth.hh"
#include <optional>

namespace vkr::exec {

struct OffscreenTargetDesc {
  ColorAttachmentDesc color{};
  std::optional<DepthAttachmentDesc> depth{};
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

  [[nodiscard]] auto width() const noexcept -> uint32_t {
    return desc_.color.width;
  }

  [[nodiscard]] auto height() const noexcept -> uint32_t {
    return desc_.color.height;
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

  // helpers
private:
  void validate() const;
};

} // namespace vkr::exec
