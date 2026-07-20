#define STB_IMAGE_IMPLEMENTATION
#include "vkr/scene/material/texture.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/buffer.hh"
#include "vkr/resource/gpu/image_commands.hh"
#include <cstddef>
#include <cstring>
#include <memory>
#include <stb_image.h>

namespace vkr::scene {

namespace {

struct StbiImageDeleter {
  void operator()(stbi_uc *pixels) const noexcept { stbi_image_free(pixels); }
};

using StbiImage = std::unique_ptr<stbi_uc, StbiImageDeleter>;

} // namespace

Texture::Texture(const core::Device &device,
                 const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool),
      image_(std::make_unique<resource::Image>(device_)),
      image_view_(std::make_unique<resource::ImageView>(device_)),
      sampler_(std::make_unique<resource::Sampler>(device_)) {}

Texture::~Texture() { destroy(); }

void Texture::update(const TextureDesc &desc) {
  desc_ = desc;
  create();
}

void Texture::create() {
  destroy();

  if (!desc_.isValid()) {
    VKR_RES_ERROR("TextureDesc is invalid");
  }

  if (!desc_.filePath.empty()) {
    createFromFile();
  } else {
    createEmpty();
  }

  createViewAndSampler();
}

void Texture::createEmpty() {
  image_->update(desc_.image);

  if (desc_.layout != VK_IMAGE_LAYOUT_UNDEFINED &&
      desc_.layout != desc_.image.layout) {
    resource::ImageCommands::transitionLayout(device_, command_pool_, *image_,
                                              desc_.image.layout,
                                              desc_.layout);
  }
}

void Texture::createFromFile() {
  int loadedWidth{0};
  int loadedHeight{0};
  int loadedChannels{0};

  const int desiredChannels = desc_.forceRgba ? STBI_rgb_alpha : 0;
  StbiImage pixels{stbi_load(desc_.filePath.c_str(), &loadedWidth,
                             &loadedHeight, &loadedChannels, desiredChannels)};

  if (!pixels) {
    VKR_RES_ERROR("Failed to load texture image from file: {}", desc_.filePath);
  }

  const uint32_t width = static_cast<uint32_t>(loadedWidth);
  const uint32_t height = static_cast<uint32_t>(loadedHeight);
  const uint32_t channels =
      desc_.forceRgba ? 4U : static_cast<uint32_t>(loadedChannels);

  if (width == 0 || height == 0 || channels == 0) {
    VKR_RES_ERROR("Loaded image '{}' has invalid size/channels",
                  desc_.filePath);
  }

  const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) *
                                 static_cast<VkDeviceSize>(height) *
                                 static_cast<VkDeviceSize>(channels);

  resource::Buffer staging{device_, imageSize,
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
  staging.write(pixels.get(), imageSize);

  auto imageDesc = desc_.image;
  imageDesc.width = width;
  imageDesc.height = height;
  imageDesc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  image_->update(imageDesc);

  resource::ImageCommands::transitionLayout(
      device_, command_pool_, *image_, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  resource::ImageCommands::copyBufferToImage(device_, command_pool_, staging,
                                             *image_, 1, channels);

  if (desc_.layout != VK_IMAGE_LAYOUT_UNDEFINED &&
      desc_.layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    resource::ImageCommands::transitionLayout(
        device_, command_pool_, *image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        desc_.layout);
  }
}

void Texture::createViewAndSampler() {
  image_view_->update(desc_.useDefaultView
                          ? resource::ImageViewDesc::fromImage(*image_)
                          : desc_.view);
  if (desc_.createSampler) {
    sampler_->update(desc_.sampler);
  } else {
    sampler_->destroy();
  }
}

void Texture::destroy() {
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
