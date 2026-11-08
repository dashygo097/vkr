#include "vkr/pipeline/attachments/color.hh"
#include "vkr/logger.hh"

namespace vkr::pipe {

ColorAttachment::ColorAttachment(const core::Device &device)
    : device_(device), image_(device), image_view_(device) {}

void ColorAttachment::create() {
  if (desc_.format == VK_FORMAT_UNDEFINED) {
    VKR_RES_ERROR("ColorAttachment has undefined format");
  }

  if (desc_.width == 0 || desc_.height == 0) {
    VKR_RES_ERROR("ColorAttachment has invalid size: {}x{}", desc_.width,
                  desc_.height);
  }

  auto imageDesc =
      res::ImageDesc::colorAttachment(desc_.width, desc_.height, desc_.format);

  imageDesc.usage = desc_.usage;

  destory();
  image_.update(imageDesc);
  image_.layout(desc_.finalLayout == VK_IMAGE_LAYOUT_UNDEFINED
                     ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                     : desc_.finalLayout);
  image_view_.update(
      res::ImageViewDesc::color2D(image_.image(), desc_.format));

  if (desc_.createSampler) {
    sampler_.emplace(device_).update(desc_.sampler);
  }
}

void ColorAttachment::destory() {
  sampler_.reset();
  image_view_.destroy();
  image_.destroy();
}

void ColorAttachment::update(const ColorAttachmentDesc &desc) {
  desc_ = desc;
  create();
}

} // namespace vkr::pipe
