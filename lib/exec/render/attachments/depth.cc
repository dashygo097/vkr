#include "vkr/exec/render/attachments/depth.hh"
#include "vkr/logger.hh"
#include <vulkan/vulkan_core.h>

namespace vkr::exec {
DepthAttachment::DepthAttachment(const core::Device &device,
                                 const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool) {
  image_ = std::make_unique<resource::Image>(device_);
  image_view_ = std::make_unique<resource::ImageView>(device_);
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

  auto imageDesc = resource::ImageDesc::depthAttachment(
      desc_.width, desc_.height, desc_.format);

  imageDesc.usage = desc_.usage;

  image_->update(imageDesc);
  image_->setLayout(desc_.finalLayout == VK_IMAGE_LAYOUT_UNDEFINED
                        ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                        : desc_.finalLayout);
  image_view_->update(
      resource::ImageViewDesc::depth2D(image_->image(), desc_.format));

  if (desc_.createSampler) {
    if (!sampler_) {
      sampler_ = std::make_unique<resource::Sampler>(device_);
    }

    sampler_->update(desc_.sampler);
  } else {
    sampler_.reset();
  }
}

void DepthAttachment::destory() {
  sampler_.reset();

  if (image_view_) {
    image_view_->destroy();
  }

  if (image_) {
    image_->destroy();
  }
}

void DepthAttachment::update(const DepthAttachmentDesc &desc) {
  desc_ = desc;
  create();
}

} // namespace vkr::exec
