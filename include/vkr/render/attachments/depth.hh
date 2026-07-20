#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/image/image.hh"
#include "vkr/resource/image/image_view.hh"

namespace vkr::render {

struct DepthAttachmentDesc {
  uint32_t width{};
  uint32_t height{};
  VkFormat format{VK_FORMAT_UNDEFINED};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return width != 0 && height != 0 && format != VK_FORMAT_UNDEFINED;
  }
};

class DepthAttachment {
public:
  explicit DepthAttachment(const core::Device &device,
                           const core::CommandPool &commandPool);

  ~DepthAttachment();

  DepthAttachment(const DepthAttachment &) = delete;
  auto operator=(const DepthAttachment &) -> DepthAttachment & = delete;

  [[nodiscard]] auto desc() const noexcept -> const DepthAttachmentDesc & {
    return desc_;
  }

  void create();
  void destory();
  void update(const DepthAttachmentDesc &desc);

  [[nodiscard]] auto image() const noexcept -> VkImage {
    return image_->image();
  }
  [[nodiscard]] auto imageView() const noexcept -> VkImageView {
    return image_view_->imageView();
  }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  DepthAttachmentDesc desc_{};
  std::unique_ptr<resource::Image> image_;
  std::unique_ptr<resource::ImageView> image_view_;
};

} // namespace vkr::render
