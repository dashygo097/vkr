#include "vkr/pipeline/attachments/depth.hh"
#include "vkr/logger.hh"
#include <vulkan/vulkan_core.h>

namespace vkr::pipe {
DepthAttachment::DepthAttachment(const core::Device &device)
    : device_(device), image_(device), image_view_(device) {}

void DepthAttachment::create() {
  if (desc_.format == VK_FORMAT_UNDEFINED) {
    VKR_RES_ERROR("DepthAttachment has undefined format");
  }

  if (desc_.width == 0 || desc_.height == 0) {
    VKR_RES_ERROR("DepthAttachment has invalid size: {}x{}", desc_.width,
                  desc_.height);
  }

  auto imageDesc =
      res::ImageDesc::depthAttachment(desc_.width, desc_.height, desc_.format);

  imageDesc.usage = desc_.usage;

  destory();
  image_.update(imageDesc);
  image_.layout(desc_.finalLayout == VK_IMAGE_LAYOUT_UNDEFINED
                     ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                     : desc_.finalLayout);
  image_view_.update(
      res::ImageViewDesc::depth2D(image_.image(), desc_.format));

  if (desc_.createSampler) {
    sampler_.emplace(device_).update(desc_.sampler);
  }
}

void DepthAttachment::destory() {
  sampler_.reset();
  image_view_.destroy();
  image_.destroy();
}

void DepthAttachment::update(const DepthAttachmentDesc &desc) {
  desc_ = desc;
  create();
}

} // namespace vkr::pipe
