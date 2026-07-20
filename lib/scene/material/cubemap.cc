#include "vkr/scene/material/cubemap.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffer/buffer.hh"
#include "vkr/resource/image/image_commands.hh"
#include <cstddef>
#include <cstring>
#include <memory>
#include <stb_image.h>

namespace vkr::scene {

namespace {

constexpr uint32_t FaceCount = 6;

struct StbiImageDeleter {
  void operator()(stbi_uc *pixels) const noexcept { stbi_image_free(pixels); }
};

using StbiImage = std::unique_ptr<stbi_uc, StbiImageDeleter>;

} // namespace

Cubemap::Cubemap(const core::Device &device,
                 const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool),
      image_(std::make_unique<resource::Image>(device_)),
      image_view_(std::make_unique<resource::ImageView>(device_)),
      sampler_(std::make_unique<resource::Sampler>(device_)) {}

Cubemap::~Cubemap() { destroy(); }

void Cubemap::update(const CubemapDesc &desc) {
  desc_ = desc;
  create();
}

void Cubemap::create() {
  destroy();

  if (!desc_.isValid()) {
    VKR_RES_ERROR("CubemapDesc is invalid");
  }

  std::array<StbiImage, FaceCount> facePixels{};
  int loadedWidth{0};
  int loadedHeight{0};
  int loadedChannels{0};
  const int desiredChannels = desc_.forceRgba ? STBI_rgb_alpha : 0;

  for (uint32_t face = 0; face < FaceCount; ++face) {
    int faceWidth{0};
    int faceHeight{0};
    int faceChannels{0};

    facePixels[face].reset(stbi_load(desc_.facePaths[face].c_str(),
                                     &faceWidth, &faceHeight, &faceChannels,
                                     desiredChannels));

    if (!facePixels[face]) {
      VKR_RES_ERROR("Failed to load cubemap face {} from file: {}", face,
                    desc_.facePaths[face]);
    }

    if (faceWidth <= 0 || faceHeight <= 0 || faceChannels <= 0) {
      VKR_RES_ERROR("Loaded cubemap face '{}' has invalid size/channels",
                    desc_.facePaths[face]);
    }

    if (faceWidth != faceHeight) {
      VKR_RES_ERROR("Cubemap face '{}' must be square, got {}x{}",
                    desc_.facePaths[face], faceWidth, faceHeight);
    }

    const int effectiveChannels =
        desc_.forceRgba ? STBI_rgb_alpha : faceChannels;

    if (face == 0) {
      loadedWidth = faceWidth;
      loadedHeight = faceHeight;
      loadedChannels = effectiveChannels;
      continue;
    }

    if (faceWidth != loadedWidth || faceHeight != loadedHeight ||
        effectiveChannels != loadedChannels) {
      VKR_RES_ERROR("Cubemap face '{}' size/channels mismatch",
                    desc_.facePaths[face]);
    }
  }

  const uint32_t width = static_cast<uint32_t>(loadedWidth);
  const uint32_t height = static_cast<uint32_t>(loadedHeight);
  const uint32_t channels = static_cast<uint32_t>(loadedChannels);

  const VkDeviceSize faceSize = static_cast<VkDeviceSize>(width) *
                                static_cast<VkDeviceSize>(height) *
                                static_cast<VkDeviceSize>(channels);
  const VkDeviceSize imageSize = faceSize * FaceCount;

  resource::Buffer staging{device_, imageSize,
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  auto *data = static_cast<std::byte *>(staging.map(imageSize));
  for (uint32_t face = 0; face < FaceCount; ++face) {
    std::memcpy(data + faceSize * face, facePixels[face].get(),
                static_cast<size_t>(faceSize));
  }
  staging.unmap();

  resource::ImageDesc imageDesc =
      resource::ImageDesc::sampled2D(width, height, desc_.format);
  imageDesc.arrayLayers = FaceCount;
  imageDesc.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  imageDesc.defaultViewType = VK_IMAGE_VIEW_TYPE_CUBE;
  imageDesc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  image_->update(imageDesc);

  resource::ImageCommands::transitionLayout(
      device_, command_pool_, *image_, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  resource::ImageCommands::copyBufferToImage(device_, command_pool_, staging,
                                             *image_, FaceCount, channels);

  if (desc_.layout != VK_IMAGE_LAYOUT_UNDEFINED &&
      desc_.layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    resource::ImageCommands::transitionLayout(
        device_, command_pool_, *image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        desc_.layout);
  }

  image_view_->update(resource::ImageViewDesc::fromImage(*image_));
  sampler_->update(desc_.sampler);
}

void Cubemap::destroy() {
  if (sampler_) {
    sampler_->destroy();
  }

  if (image_view_) {
    image_view_->destroy();
  }

  if (image_) {
    image_->destroy();
  }
}

} // namespace vkr::scene
