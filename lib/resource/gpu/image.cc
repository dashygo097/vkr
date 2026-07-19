#define STB_IMAGE_IMPLEMENTATION
#include "vkr/resource/gpu/image.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/buffer.hh"
#include <array>
#include <cstddef>
#include <cstring>
#include <memory>
#include <stb_image.h>
#include <vector>

namespace vkr::resource {

namespace {

struct StbiImageDeleter {
  void operator()(stbi_uc *pixels) const noexcept { stbi_image_free(pixels); }
};

using StbiImage = std::unique_ptr<stbi_uc, StbiImageDeleter>;

} // namespace

Image::Image(const core::Device &device, const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool) {}

Image::~Image() { destroy(); }

void Image::update(const ImageDesc &desc) {
  destroy();
  desc_ = desc;
  create();
}

void Image::create() {
  if (desc_.format == VK_FORMAT_UNDEFINED) {
    VKR_RES_ERROR("ImageDesc has undefined format");
  }

  if (desc_.isCubemap) {
    createCubemapFromFiles();
    return;
  }

  if (desc_.hasFile()) {
    createFromFile();
    return;
  }

  createEmpty();
}

void Image::destroy() {
  if (vk_image_ != VK_NULL_HANDLE) {
    vkDestroyImage(device_.device(), vk_image_, nullptr);
    vk_image_ = VK_NULL_HANDLE;
  }

  if (vk_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(device_.device(), vk_memory_, nullptr);
    vk_memory_ = VK_NULL_HANDLE;
  }

  width_ = 0;
  height_ = 0;
  channels_ = 0;
  layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
}

void Image::createEmpty() {
  if (desc_.width == 0 || desc_.height == 0) {
    VKR_RES_ERROR("ImageDesc has invalid empty image size: {}x{}", desc_.width,
                  desc_.height);
  }

  if (desc_.usage == 0) {
    VKR_RES_ERROR("ImageDesc has no image usage flags");
  }

  width_ = desc_.width;
  height_ = desc_.height;
  channels_ = 0;

  createImageObject(desc_.width, desc_.height, desc_.format, desc_.tiling,
                    desc_.usage, desc_.memoryProperties, desc_.mipLevels,
                    desc_.arrayLayers, desc_.samples);

  layout_ = desc_.initialLayout;

  if (desc_.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED &&
      desc_.finalLayout != desc_.initialLayout) {
    transitionImageLayout(vk_image_, desc_.format, desc_.aspectMask,
                          desc_.initialLayout, desc_.finalLayout);
    layout_ = desc_.finalLayout;
  }
}

void Image::createFromFile() {
  int loadedWidth{0};
  int loadedHeight{0};
  int loadedChannels{0};

  const int desiredChannels = desc_.forceRgba ? STBI_rgb_alpha : 0;
  StbiImage pixels{stbi_load(desc_.filePath.c_str(), &loadedWidth,
                             &loadedHeight, &loadedChannels, desiredChannels)};

  if (!pixels) {
    VKR_RES_ERROR("Failed to load texture image from file: {}", desc_.filePath);
  }

  width_ = static_cast<uint32_t>(loadedWidth);
  height_ = static_cast<uint32_t>(loadedHeight);
  channels_ = desc_.forceRgba ? 4U : static_cast<uint32_t>(loadedChannels);

  if (width_ == 0 || height_ == 0 || channels_ == 0) {
    VKR_RES_ERROR("Loaded image '{}' has invalid size/channels",
                  desc_.filePath);
  }

  const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width_) *
                                 static_cast<VkDeviceSize>(height_) *
                                 static_cast<VkDeviceSize>(channels_);

  Buffer staging{device_, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
  staging.write(pixels.get(), imageSize);

  const VkImageUsageFlags usage = desc_.usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  createImageObject(width_, height_, desc_.format, desc_.tiling, usage,
                    desc_.memoryProperties, desc_.mipLevels, desc_.arrayLayers,
                    desc_.samples);

  transitionImageLayout(vk_image_, desc_.format, desc_.aspectMask,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(staging.buffer(), vk_image_, width_, height_);

  if (desc_.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED &&
      desc_.finalLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    transitionImageLayout(vk_image_, desc_.format, desc_.aspectMask,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          desc_.finalLayout);
    layout_ = desc_.finalLayout;
  } else {
    layout_ = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  }
}

void Image::createCubemapFromFiles() {
  if (!desc_.hasCubemapFiles()) {
    VKR_RES_ERROR("Cubemap ImageDesc must provide exactly 6 face paths");
  }

  if (desc_.mipLevels != 1) {
    VKR_RES_ERROR("Cubemap file loading currently supports only 1 mip level");
  }

  constexpr uint32_t FaceCount = 6;
  std::array<StbiImage, FaceCount> facePixels{};

  int loadedWidth{0};
  int loadedHeight{0};
  int loadedChannels{0};
  const int desiredChannels = desc_.forceRgba ? STBI_rgb_alpha : 0;

  for (uint32_t face = 0; face < FaceCount; ++face) {
    int faceWidth{0};
    int faceHeight{0};
    int faceChannels{0};

    facePixels[face].reset(stbi_load(desc_.cubeFacePaths[face].c_str(),
                                     &faceWidth, &faceHeight, &faceChannels,
                                     desiredChannels));

    if (!facePixels[face]) {
      VKR_RES_ERROR("Failed to load cubemap face {} from file: {}", face,
                    desc_.cubeFacePaths[face]);
    }

    if (faceWidth <= 0 || faceHeight <= 0 || faceChannels <= 0) {
      VKR_RES_ERROR("Loaded cubemap face '{}' has invalid size/channels",
                    desc_.cubeFacePaths[face]);
    }

    if (faceWidth != faceHeight) {
      VKR_RES_ERROR("Cubemap face '{}' must be square, got {}x{}",
                    desc_.cubeFacePaths[face], faceWidth, faceHeight);
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
                    desc_.cubeFacePaths[face]);
    }
  }

  width_ = static_cast<uint32_t>(loadedWidth);
  height_ = static_cast<uint32_t>(loadedHeight);
  channels_ = static_cast<uint32_t>(loadedChannels);

  const VkDeviceSize faceSize = static_cast<VkDeviceSize>(width_) *
                                static_cast<VkDeviceSize>(height_) *
                                static_cast<VkDeviceSize>(channels_);
  const VkDeviceSize imageSize = faceSize * FaceCount;

  Buffer staging{device_, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  auto *data = static_cast<std::byte *>(staging.map(imageSize));

  for (uint32_t face = 0; face < FaceCount; ++face) {
    std::memcpy(data + faceSize * face, facePixels[face].get(),
                static_cast<size_t>(faceSize));
  }
  staging.unmap();

  const VkImageUsageFlags usage = desc_.usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  createImageObject(width_, height_, desc_.format, desc_.tiling, usage,
                    desc_.memoryProperties, desc_.mipLevels, FaceCount,
                    desc_.samples);

  transitionImageLayout(vk_image_, desc_.format, desc_.aspectMask,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImageLayers(staging.buffer(), vk_image_, width_, height_,
                          FaceCount);

  if (desc_.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED &&
      desc_.finalLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    transitionImageLayout(vk_image_, desc_.format, desc_.aspectMask,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          desc_.finalLayout);
    layout_ = desc_.finalLayout;
  } else {
    layout_ = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  }
}

void Image::createImageObject(uint32_t width, uint32_t height, VkFormat format,
                              VkImageTiling tiling, VkImageUsageFlags usage,
                              VkMemoryPropertyFlags properties,
                              uint32_t mipLevels, uint32_t arrayLayers,
                              VkSampleCountFlagBits samples) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.flags = desc_.flags;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = mipLevels;
  imageInfo.arrayLayers = arrayLayers;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = samples;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device_.device(), &imageInfo, nullptr, &vk_image_) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to create image");
  }

  VkMemoryRequirements memRequirements{};
  vkGetImageMemoryRequirements(device_.device(), vk_image_, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = Buffer::findMemoryType(
      memRequirements.memoryTypeBits, properties, device_);

  if (vkAllocateMemory(device_.device(), &allocInfo, nullptr, &vk_memory_) !=
      VK_SUCCESS) {
    vkDestroyImage(device_.device(), vk_image_, nullptr);
    vk_image_ = VK_NULL_HANDLE;
    VKR_RES_ERROR("Failed to allocate image memory");
  }

  if (vkBindImageMemory(device_.device(), vk_image_, vk_memory_, 0) !=
      VK_SUCCESS) {
    vkDestroyImage(device_.device(), vk_image_, nullptr);
    vkFreeMemory(device_.device(), vk_memory_, nullptr);
    vk_image_ = VK_NULL_HANDLE;
    vk_memory_ = VK_NULL_HANDLE;
    VKR_RES_ERROR("Failed to bind image memory");
  }
}

void Image::transitionImageLayout(VkImage image, VkFormat format,
                                  VkImageAspectFlags aspectMask,
                                  VkImageLayout oldLayout,
                                  VkImageLayout newLayout) {
  (void)format;

  if (oldLayout == newLayout) {
    return;
  }

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = aspectMask;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = desc_.mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = desc_.arrayLayers;

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
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                       VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                       VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_GENERAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask =
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask =
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_GENERAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask =
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_GENERAL) {
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask =
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL &&
             newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask =
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  } else {
    VKR_RES_ERROR("Unsupported image layout transition: {} -> {}",
                  static_cast<int>(oldLayout), static_cast<int>(newLayout));
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

void Image::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                              uint32_t height) {
  copyBufferToImageLayers(buffer, image, width, height, 1);
}

void Image::copyBufferToImageLayers(VkBuffer buffer, VkImage image,
                                    uint32_t width, uint32_t height,
                                    uint32_t layers) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  const VkDeviceSize layerSize = static_cast<VkDeviceSize>(width) *
                                 static_cast<VkDeviceSize>(height) *
                                 static_cast<VkDeviceSize>(channels_);
  std::vector<VkBufferImageCopy> regions{};
  regions.reserve(layers);

  for (uint32_t layer = 0; layer < layers; ++layer) {
    VkBufferImageCopy region{};
    region.bufferOffset = layerSize * layer;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = desc_.aspectMask;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    regions.push_back(region);
  }

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         static_cast<uint32_t>(regions.size()), regions.data());

  endSingleTimeCommands(commandBuffer);
}

auto Image::beginSingleTimeCommands() -> VkCommandBuffer {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = command_pool_.commandPool();
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
  if (vkAllocateCommandBuffers(device_.device(), &allocInfo, &commandBuffer) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to allocate single-time command buffer");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to begin single-time command buffer");
  }

  return commandBuffer;
}

void Image::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to end single-time command buffer");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(command_pool_.queue(), 1, &submitInfo, VK_NULL_HANDLE) !=
      VK_SUCCESS) {
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to submit single-time command buffer");
  }

  vkQueueWaitIdle(command_pool_.queue());
  vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                       &commandBuffer);
}

} // namespace vkr::resource
