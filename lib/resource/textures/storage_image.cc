#include "vkr/resource/textures/storage_image.hh"
#include "vkr/logger.hh"

namespace vkr::res {

StorageImage::StorageImage(const core::Device &device,
                           const core::CommandPool &commandPool)
    : Texture(device), command_pool_(commandPool) {}

void StorageImage::update(const StorageImageDesc &desc) {
  if (!desc.isValid()) {
    VKR_RES_ERROR("StorageImage requires a valid 2D storage image descriptor");
  }
  desc_ = desc;
  Texture::create(desc.image);
  if (desc.layout != VK_IMAGE_LAYOUT_UNDEFINED) {
    transition(command_pool_, desc.layout);
  }
}

} // namespace vkr::res
