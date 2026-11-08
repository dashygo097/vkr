#include "vkr/resource/textures/texture2d.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffer.hh"
#include "vkr/util/image_loader.hh"

namespace vkr::res {

Texture2D::Texture2D(const core::Device &device,
                     const core::CommandPool &commandPool)
    : Texture(device), device_(device), command_pool_(commandPool) {}

void Texture2D::update(const Texture2DDesc &desc) {
  if (!desc.isValid()) {
    VKR_RES_ERROR("Texture2DDesc is invalid");
  }
  desc_ = desc;
  create();
}

void Texture2D::create() {
  if (!desc_.isValid() || desc_.image.type != VK_IMAGE_TYPE_2D ||
      desc_.image.depth != 1 || desc_.image.arrayLayers != 1 ||
      desc_.image.defaultViewType != VK_IMAGE_VIEW_TYPE_2D) {
    VKR_RES_ERROR("Texture2D requires a valid single-layer 2D image");
  }
  const auto view = desc_.useDefaultView
                        ? std::optional<ImageViewDesc>{}
                        : std::optional<ImageViewDesc>{desc_.view};
  if (desc_.filePath.empty()) {
    Texture::create(desc_.image, view);
    if (desc_.layout != VK_IMAGE_LAYOUT_UNDEFINED) {
      transition(command_pool_, desc_.layout);
    }
  } else {
    const auto data = util::loadImage(desc_.filePath, desc_.forceRgba);
    auto imageDesc = desc_.image;
    imageDesc.width = data.width;
    imageDesc.height = data.height;
    imageDesc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    Buffer staging(device_);
    staging.update(data.pixels.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    staging.write(data.pixels.data(), data.pixels.size());
    Texture::create(imageDesc, view);
    upload(command_pool_, staging, data.channels, desc_.layout);
  }
  if (desc_.createSampler) {
    sampler_.emplace(device_).update(desc_.sampler);
  } else {
    sampler_.reset();
  }
}

void Texture2D::destroy() noexcept {
  sampler_.reset();
  Texture::destroy();
}

} // namespace vkr::res
