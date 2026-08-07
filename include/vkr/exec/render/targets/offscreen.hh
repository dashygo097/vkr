#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/exec/render/attachments/color.hh"
#include "vkr/exec/render/attachments/depth.hh"
#include <optional>
#include <utility>

namespace vkr::exec {

struct OffscreenTargetDesc {
  bool colorEnabled{true};
  ColorAttachmentDesc color{};
  std::optional<DepthAttachmentDesc> depth{};

  auto colorAttachment(ColorAttachmentDesc desc) -> OffscreenTargetDesc & {
    color = std::move(desc);
    colorEnabled = true;
    return *this;
  }

  auto colorAttachment(uint32_t width, uint32_t height, VkFormat format)
      -> OffscreenTargetDesc & {
    return colorAttachment(
        ColorAttachmentDesc::attachment(width, height, format));
  }

  auto sampledColor(uint32_t width, uint32_t height, VkFormat format)
      -> OffscreenTargetDesc & {
    return colorAttachment(
        ColorAttachmentDesc::sampled2D(width, height, format));
  }

  auto sampledColor(bool enabled = true) -> OffscreenTargetDesc & {
    color.sampled(enabled);
    colorEnabled = true;
    return *this;
  }

  auto disableColor() noexcept -> OffscreenTargetDesc & {
    colorEnabled = false;
    return *this;
  }

  auto depthAttachment(DepthAttachmentDesc desc) -> OffscreenTargetDesc & {
    depth = std::move(desc);
    return *this;
  }

  auto depthAttachment(uint32_t width, uint32_t height, VkFormat format)
      -> OffscreenTargetDesc & {
    return depthAttachment(
        DepthAttachmentDesc::attachment(width, height, format));
  }

  auto sampledDepth(uint32_t width, uint32_t height, VkFormat format)
      -> OffscreenTargetDesc & {
    return depthAttachment(
        DepthAttachmentDesc::sampled2D(width, height, format));
  }

  auto sampledDepth(bool enabled = true) -> OffscreenTargetDesc & {
    ensureDepth();
    depth->sampled(enabled);
    return *this;
  }

  auto disableDepth() noexcept -> OffscreenTargetDesc & {
    depth.reset();
    return *this;
  }

  [[nodiscard]] auto hasColor() const noexcept -> bool { return colorEnabled; }

  [[nodiscard]] auto hasDepth() const noexcept -> bool {
    return depth.has_value();
  }

  [[nodiscard]] auto width() const noexcept -> uint32_t {
    return colorEnabled ? color.width : (depth ? depth->width : 0U);
  }

  [[nodiscard]] auto height() const noexcept -> uint32_t {
    return colorEnabled ? color.height : (depth ? depth->height : 0U);
  }

  [[nodiscard]] static auto colorOnly(uint32_t width, uint32_t height,
                                      VkFormat format) -> OffscreenTargetDesc {
    OffscreenTargetDesc desc{};
    return desc.colorAttachment(width, height, format).disableDepth();
  }

  [[nodiscard]] static auto sampledColorOnly(uint32_t width, uint32_t height,
                                             VkFormat format)
      -> OffscreenTargetDesc {
    OffscreenTargetDesc desc{};
    return desc.sampledColor(width, height, format).disableDepth();
  }

  [[nodiscard]] static auto colorDepth(uint32_t width, uint32_t height,
                                       VkFormat colorFormat,
                                       VkFormat depthFormat)
      -> OffscreenTargetDesc {
    OffscreenTargetDesc desc{};
    return desc.colorAttachment(width, height, colorFormat)
        .depthAttachment(width, height, depthFormat);
  }

  [[nodiscard]] static auto sampledColorDepth(uint32_t width, uint32_t height,
                                              VkFormat colorFormat,
                                              VkFormat depthFormat)
      -> OffscreenTargetDesc {
    OffscreenTargetDesc desc{};
    return desc.sampledColor(width, height, colorFormat)
        .depthAttachment(width, height, depthFormat);
  }

  [[nodiscard]] static auto
  shadowMap(uint32_t width, uint32_t height,
            VkFormat depthFormat = VK_FORMAT_D32_SFLOAT)
      -> OffscreenTargetDesc {
    OffscreenTargetDesc desc{};
    return desc.disableColor().depthAttachment(
        DepthAttachmentDesc::shadowMap(width, height, depthFormat));
  }

  void ensureDepth() {
    if (!depth) {
      depth = DepthAttachmentDesc::attachment(color.width, color.height,
                                              VK_FORMAT_D32_SFLOAT);
    }
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

  [[nodiscard]] auto hasColor() const noexcept -> bool {
    return color_ != nullptr;
  }

  [[nodiscard]] auto width() const noexcept -> uint32_t {
    return desc_.width();
  }

  [[nodiscard]] auto height() const noexcept -> uint32_t {
    return desc_.height();
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
