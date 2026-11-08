#include "vkr/resource/textures/cubemap.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffer.hh"
#include "vkr/util/image_loader.hh"
#include <limits>

namespace vkr::res {

Cubemap::Cubemap(const core::Device &device,
                 const core::CommandPool &commandPool)
    : Texture(device), device_(device), command_pool_(commandPool) {}

void Cubemap::update(const CubemapDesc &desc) {
  if (!desc.isValid()) {
    VKR_RES_ERROR("CubemapDesc is invalid");
  }
  desc_ = desc;
  create();
}

void Cubemap::create() {
  std::array<util::ImageData, 6> faces{};
  for (size_t face = 0; face < faces.size(); ++face) {
    faces[face] = util::loadImage(desc_.facePaths[face], desc_.forceRgba);
    const auto &data = faces[face];
    if (data.width != data.height || data.width != faces.front().width ||
        data.height != faces.front().height ||
        data.channels != faces.front().channels) {
      VKR_RES_ERROR("Cubemap face '{}' must be square with matching dimensions "
                    "and channels",
                    desc_.facePaths[face]);
    }
  }
  const auto &first = faces.front();
  if (first.pixels.size() >
      std::numeric_limits<VkDeviceSize>::max() / faces.size()) {
    VKR_RES_ERROR("Cubemap data exceeds the supported upload size");
  }
  const VkDeviceSize faceBytes = first.pixels.size();
  Buffer staging(device_);
  staging.update(faceBytes * faces.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 true);
  for (size_t face = 0; face < faces.size(); ++face) {
    staging.write(faces[face].pixels.data(), faceBytes, faceBytes * face);
  }

  auto imageDesc =
      ImageDesc::sampled2D(first.width, first.height, desc_.format);
  imageDesc.arrayLayers = static_cast<uint32_t>(faces.size());
  imageDesc.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  imageDesc.defaultViewType = VK_IMAGE_VIEW_TYPE_CUBE;
  Texture::create(imageDesc);
  upload(command_pool_, staging, first.channels, desc_.layout);
  if (desc_.createSampler) {
    sampler_.emplace(device_).update(desc_.sampler);
  } else {
    sampler_.reset();
  }
}

void Cubemap::destroy() noexcept {
  sampler_.reset();
  Texture::destroy();
}

} // namespace vkr::res
