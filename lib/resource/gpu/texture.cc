#define STB_IMAGE_IMPLEMENTATION
#include "vkr/resource/gpu/texture.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/buffer.hh"
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

Texture::Texture(const core::Device &device,
                 const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool),
      image_(std::make_unique<Image>(device_)),
      image_view_(std::make_unique<ImageView>(device_)),
      sampler_(std::make_unique<Sampler>(device_)) {}

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

  if (desc_.isCubemap) {
    createCubemapFromFiles();
  } else if (!desc_.filePath.empty()) {
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
    transitionImageLayout(desc_.image.layout, desc_.layout);
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

  Buffer staging{device_, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
  staging.write(pixels.get(), imageSize);

  auto imageDesc = desc_.image;
  imageDesc.width = width;
  imageDesc.height = height;
  imageDesc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  image_->update(imageDesc);

  transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(staging.buffer(), 1, channels);

  if (desc_.layout != VK_IMAGE_LAYOUT_UNDEFINED &&
      desc_.layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, desc_.layout);
  }
}

void Texture::createCubemapFromFiles() {
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

  const uint32_t width = static_cast<uint32_t>(loadedWidth);
  const uint32_t height = static_cast<uint32_t>(loadedHeight);
  const uint32_t channels = static_cast<uint32_t>(loadedChannels);

  const VkDeviceSize faceSize = static_cast<VkDeviceSize>(width) *
                                static_cast<VkDeviceSize>(height) *
                                static_cast<VkDeviceSize>(channels);
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

  auto imageDesc = desc_.image;
  imageDesc.width = width;
  imageDesc.height = height;
  imageDesc.arrayLayers = FaceCount;
  imageDesc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  image_->update(imageDesc);

  transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(staging.buffer(), FaceCount, channels);

  if (desc_.layout != VK_IMAGE_LAYOUT_UNDEFINED &&
      desc_.layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, desc_.layout);
  }
}

void Texture::createViewAndSampler() {
  image_view_->update(desc_.useDefaultView ? ImageViewDesc::fromImage(*image_)
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

void Texture::transitionImageLayout(VkImageLayout oldLayout,
                                    VkImageLayout newLayout) {
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
  barrier.image = image_->image();
  barrier.subresourceRange.aspectMask = image_->desc().aspectMask;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = image_->desc().mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = image_->desc().arrayLayers;

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
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
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
  } else {
    VKR_RES_ERROR("Unsupported texture image layout transition: {} -> {}",
                  static_cast<int>(oldLayout), static_cast<int>(newLayout));
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
  image_->setLayout(newLayout);
}

void Texture::copyBufferToImage(VkBuffer buffer, uint32_t layers,
                                uint32_t channels) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  const VkDeviceSize layerSize = static_cast<VkDeviceSize>(image_->width()) *
                                 static_cast<VkDeviceSize>(image_->height()) *
                                 static_cast<VkDeviceSize>(channels);
  std::vector<VkBufferImageCopy> regions{};
  regions.reserve(layers);

  for (uint32_t layer = 0; layer < layers; ++layer) {
    VkBufferImageCopy region{};
    region.bufferOffset = layerSize * layer;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = image_->desc().aspectMask;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {image_->width(), image_->height(), 1};
    regions.push_back(region);
  }

  vkCmdCopyBufferToImage(commandBuffer, buffer, image_->image(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         static_cast<uint32_t>(regions.size()), regions.data());

  endSingleTimeCommands(commandBuffer);
}

auto Texture::beginSingleTimeCommands() -> VkCommandBuffer {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = command_pool_.commandPool();
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
  if (vkAllocateCommandBuffers(device_.device(), &allocInfo, &commandBuffer) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to allocate texture command buffer");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to begin texture command buffer");
  }

  return commandBuffer;
}

void Texture::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to end texture command buffer");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(command_pool_.queue(), 1, &submitInfo, VK_NULL_HANDLE) !=
      VK_SUCCESS) {
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to submit texture command buffer");
  }

  vkQueueWaitIdle(command_pool_.queue());
  vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                       &commandBuffer);
}

} // namespace vkr::resource
