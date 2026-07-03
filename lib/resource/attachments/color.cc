#include "vkr/resource/attachments/color.hh"
#include "vkr/logger.hh"

namespace vkr::resource {

ColorAttachment::ColorAttachment(const core::Device &device,
                                 const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool) {
  image_ = std::make_unique<Image>(device_, command_pool_);
  image_view_ = std::make_unique<ImageView>(device_);
}

ColorAttachment::~ColorAttachment() { destory(); }

void ColorAttachment::create() {
  if (desc_.format == VK_FORMAT_UNDEFINED) {
    VKR_RES_ERROR("ColorAttachment has undefined format");
  }

  if (desc_.width == 0 || desc_.height == 0) {
    VKR_RES_ERROR("ColorAttachment has invalid size: {}x{}", desc_.width,
                  desc_.height);
  }

  auto imageDesc =
      ImageDesc::colorAttachment(desc_.width, desc_.height, desc_.format);

  imageDesc.usage = desc_.usage;
  imageDesc.finalLayout = desc_.finalLayout;

  image_->update(imageDesc);
  image_view_->update(ImageViewDesc::color2D(image_->image(), desc_.format));

  if (desc_.createSampler) {
    if (!sampler_) {
      sampler_ = std::make_unique<Sampler>(device_);
    }

    sampler_->update(desc_.sampler);
  } else {
    sampler_.reset();
  }
}

void ColorAttachment::destory() {
  sampler_.reset();

  if (image_view_) {
    image_view_->destroy();
  }

  if (image_) {
    image_->destroy();
  }
}

void ColorAttachment::update(const ColorAttachmentDesc &desc) {
  desc_ = desc;
  create();
}

} // namespace vkr::resource
