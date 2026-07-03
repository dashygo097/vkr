#include "vkr/resource/attachments/depth.hh"
#include "vkr/logger.hh"
#include <vulkan/vulkan_core.h>

namespace vkr::resource {
DepthAttachment::DepthAttachment(const core::Device &device,
                                 const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool) {
  image_ = std::make_unique<Image>(device_, command_pool_);
  image_view_ = std::make_unique<ImageView>(device_);
}

DepthAttachment::~DepthAttachment() { destory(); };

void DepthAttachment::create() {
  if (desc_.format == VK_FORMAT_UNDEFINED) {
    VKR_RES_ERROR("DepthAttachment has undefined format");
  }

  if (desc_.width == 0 || desc_.height == 0) {
    VKR_RES_ERROR("DepthAttachment has invalid size: {}x{}", desc_.width,
                  desc_.height);
  }

  image_->create();
  image_view_->create();
}

void DepthAttachment::destory() {
  image_view_->destroy();
  image_->destroy();
}

void DepthAttachment::update(const DepthAttachmentDesc &desc) {
  desc_ = desc;
  if (desc_.format == VK_FORMAT_UNDEFINED) {
    VKR_RES_ERROR("DepthAttachment has undefined format");
  }

  if (desc_.width == 0 || desc_.height == 0) {
    VKR_RES_ERROR("DepthAttachment has invalid size: {}x{}", desc_.width,
                  desc_.height);
  }

  image_->update(
      ImageDesc::depthAttachment(desc_.width, desc_.height, desc_.format));
  image_view_->update(ImageViewDesc::depth2D(image_->image(), desc_.format));
}

} // namespace vkr::resource
