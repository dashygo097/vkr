#include "vkr/resource/image/cubemap.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/buffer.hh"
#include <cstddef>
#include <cstring>
#include <memory>
#include <stb_image.h>
#include <vector>

namespace vkr::res {

namespace {

constexpr uint32_t FaceCount = 6;

struct StbiImageDeleter {
  void operator()(stbi_uc *pixels) const noexcept { stbi_image_free(pixels); }
};

using StbiImage = std::unique_ptr<stbi_uc, StbiImageDeleter>;

auto shaderStageFor(const core::CommandPool &commandPool)
    -> VkPipelineStageFlags {
  switch (commandPool.queueRole()) {
  case core::CommandQueueRole::Graphics:
    return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  case core::CommandQueueRole::Compute:
    return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  case core::CommandQueueRole::Transfer:
    VKR_RES_ERROR("Transfer command pool cannot transition cubemap for shader "
                  "access");
  }

  VKR_RES_ERROR("Unsupported command pool role for cubemap shader access");
}

void endSingleTimeCommands(const core::CommandPool &commandPool,
                           core::CommandBuffer &commands) {
  commands.end();
  const auto commandBuffer = commands.buffer();

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(commandPool.queue(), 1, &submitInfo, VK_NULL_HANDLE) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to submit cubemap upload command buffer");
  }
  if (vkQueueWaitIdle(commandPool.queue()) != VK_SUCCESS) {
    VKR_RES_ERROR("Failed to wait for cubemap upload");
  }
}

void transitionImageLayout(const core::CommandPool &commandPool, Image &image,
                           VkImageLayout oldLayout, VkImageLayout newLayout) {
  if (oldLayout == newLayout) {
    return;
  }

  auto commands = commandPool.allocate();
  commands.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  const auto commandBuffer = commands.buffer();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image.image();
  barrier.subresourceRange.aspectMask = image.desc().aspectMask;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = image.desc().mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = image.desc().arrayLayers;

  VkPipelineStageFlags sourceStage{};
  VkPipelineStageFlags destinationStage{};

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = shaderStageFor(commandPool);
  } else {
    VKR_RES_ERROR("Unsupported cubemap layout transition: {} -> {}",
                  static_cast<int>(oldLayout), static_cast<int>(newLayout));
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(commandPool, commands);
  image.layout(newLayout);
}

void copyBufferToImage(const core::CommandPool &commandPool,
                       const Buffer &buffer, Image &image, uint32_t layers,
                       uint32_t channels) {
  auto commands = commandPool.allocate();
  commands.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  const auto commandBuffer = commands.buffer();

  const VkDeviceSize layerSize = static_cast<VkDeviceSize>(image.width()) *
                                 static_cast<VkDeviceSize>(image.height()) *
                                 static_cast<VkDeviceSize>(channels);
  std::vector<VkBufferImageCopy> regions{};
  regions.reserve(layers);

  for (uint32_t layer = 0; layer < layers; ++layer) {
    VkBufferImageCopy region{};
    region.bufferOffset = layerSize * layer;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = image.desc().aspectMask;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {image.width(), image.height(), 1};
    regions.push_back(region);
  }

  vkCmdCopyBufferToImage(commandBuffer, buffer.buffer(), image.image(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         static_cast<uint32_t>(regions.size()), regions.data());

  endSingleTimeCommands(commandPool, commands);
}

} // namespace

Cubemap::Cubemap(const core::Device &device,
                 const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool),
      image_(std::make_unique<Image>(device_)),
      image_view_(std::make_unique<ImageView>(device_)),
      sampler_(std::make_unique<Sampler>(device_)) {}

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

    facePixels[face].reset(stbi_load(desc_.facePaths[face].c_str(), &faceWidth,
                                     &faceHeight, &faceChannels,
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

  auto width = static_cast<uint32_t>(loadedWidth);
  auto height = static_cast<uint32_t>(loadedHeight);
  auto channels = static_cast<uint32_t>(loadedChannels);

  const VkDeviceSize faceSize = static_cast<VkDeviceSize>(width) *
                                static_cast<VkDeviceSize>(height) *
                                static_cast<VkDeviceSize>(channels);
  const VkDeviceSize imageSize = faceSize * FaceCount;

  Buffer staging(device_);
  staging.update(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  auto *data = static_cast<std::byte *>(staging.map(imageSize));
  for (uint32_t face = 0; face < FaceCount; ++face) {
    std::memcpy(data + faceSize * face, facePixels[face].get(),
                static_cast<size_t>(faceSize));
  }
  staging.unmap();

  ImageDesc imageDesc = ImageDesc::sampled2D(width, height, desc_.format);
  imageDesc.arrayLayers = FaceCount;
  imageDesc.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  imageDesc.defaultViewType = VK_IMAGE_VIEW_TYPE_CUBE;
  imageDesc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  image_->update(imageDesc);

  transitionImageLayout(command_pool_, *image_, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(command_pool_, staging, *image_, FaceCount, channels);

  if (desc_.layout != VK_IMAGE_LAYOUT_UNDEFINED &&
      desc_.layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    transitionImageLayout(command_pool_, *image_,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, desc_.layout);
  }

  image_view_->update(ImageViewDesc::fromImage(*image_));
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

} // namespace vkr::res
