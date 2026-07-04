#pragma once

#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/swapchain.hh"
#include "vkr/resource/attachments/depth.hh"

namespace vkr::resource {

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

  [[nodiscard]] auto extent2D() const noexcept -> VkExtent2D {
    return swapchain_.extent2D();
  }

  [[nodiscard]] auto format() const noexcept -> VkFormat {
    return swapchain_.format();
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
  std::unique_ptr<DepthAttachment> depth_;
};

} // namespace vkr::resource
