#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/swapchain.hh"
#include "vkr/render/attachments/depth.hh"
#include "vkr/resource/image/image_view.hh"
#include <optional>

namespace vkr::render {

struct SwapchainTargetDesc {
  std::optional<DepthAttachmentDesc> depth{};
};

class SwapchainTarget {
public:
  SwapchainTarget(const core::Device &device,
                  const core::CommandPool &commandPool,
                  const core::Swapchain &swapchain);

  ~SwapchainTarget();

  SwapchainTarget(const SwapchainTarget &) = delete;
  auto operator=(const SwapchainTarget &) -> SwapchainTarget & = delete;

  [[nodiscard]] auto desc() const noexcept -> const SwapchainTargetDesc & {
    return desc_;
  }

  void create();
  void destory();
  void update(const SwapchainTargetDesc &desc);

  [[nodiscard]] auto images() const noexcept -> const std::vector<VkImage> & {
    return vk_color_images_;
  }

  [[nodiscard]] auto image(size_t index) const -> VkImage {
    return vk_color_images_.at(index);
  }

  [[nodiscard]] auto imageViews() const -> std::vector<VkImageView>;

  [[nodiscard]] auto imageView(size_t index) const -> VkImageView {
    return color_image_views_.at(index)->imageView();
  }

  [[nodiscard]] auto imageCount() const noexcept -> size_t {
    return vk_color_images_.size();
  }

  [[nodiscard]] auto format() const noexcept -> VkFormat {
    return swapchain_.format();
  }

  [[nodiscard]] auto width() const noexcept -> uint32_t {
    return swapchain_.width();
  }

  [[nodiscard]] auto height() const noexcept -> uint32_t {
    return swapchain_.height();
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

  [[nodiscard]] auto attachmentViews(size_t imageIndex) const
      -> std::vector<VkImageView>;

  [[nodiscard]] auto attachmentViews() const
      -> std::vector<std::vector<VkImageView>>;

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  const core::Swapchain &swapchain_;

  // components
  SwapchainTargetDesc desc_{};
  std::vector<VkImage> vk_color_images_{};
  std::vector<std::unique_ptr<resource::ImageView>> color_image_views_{};
  std::unique_ptr<DepthAttachment> depth_;
};

} // namespace vkr::render
