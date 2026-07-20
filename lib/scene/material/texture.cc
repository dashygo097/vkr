#define STB_IMAGE_IMPLEMENTATION
#include "vkr/scene/material/texture.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffer/buffer.hh"
#include <cstddef>
#include <cstring>
#include <memory>
#include <stb_image.h>
#include <vector>

namespace vkr::scene {

namespace {

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
    VKR_RES_ERROR("Transfer command pool cannot transition image for shader "
                  "access");
  }

  VKR_RES_ERROR("Unsupported command pool role for image shader access");
}

auto beginSingleTimeCommands(const core::Device &device,
                             const core::CommandPool &commandPool)
    -> VkCommandBuffer {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool.commandPool();
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
  if (vkAllocateCommandBuffers(device.device(), &allocInfo, &commandBuffer) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to allocate texture upload command buffer");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    vkFreeCommandBuffers(device.device(), commandPool.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to begin texture upload command buffer");
  }

  return commandBuffer;
}

void endSingleTimeCommands(const core::Device &device,
                           const core::CommandPool &commandPool,
                           VkCommandBuffer commandBuffer) {
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    vkFreeCommandBuffers(device.device(), commandPool.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to end texture upload command buffer");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(commandPool.queue(), 1, &submitInfo, VK_NULL_HANDLE) !=
      VK_SUCCESS) {
    vkFreeCommandBuffers(device.device(), commandPool.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to submit texture upload command buffer");
  }

  vkQueueWaitIdle(commandPool.queue());
  vkFreeCommandBuffers(device.device(), commandPool.commandPool(), 1,
                       &commandBuffer);
}

void transitionImageLayout(const core::Device &device,
                           const core::CommandPool &commandPool,
                           resource::Image &image, VkImageLayout oldLayout,
                           VkImageLayout newLayout) {
  if (oldLayout == newLayout) {
    return;
  }

  VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

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
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = shaderStageFor(commandPool);
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
    destinationStage = shaderStageFor(commandPool);
  } else {
    VKR_RES_ERROR("Unsupported texture layout transition: {} -> {}",
                  static_cast<int>(oldLayout), static_cast<int>(newLayout));
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(device, commandPool, commandBuffer);
  image.setLayout(newLayout);
}

void copyBufferToImage(const core::Device &device,
                       const core::CommandPool &commandPool,
                       const resource::Buffer &buffer, resource::Image &image,
                       uint32_t layers, uint32_t channels) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

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

  endSingleTimeCommands(device, commandPool, commandBuffer);
}

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
    transitionImageLayout(device_, command_pool_, *image_, desc_.image.layout,
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

  resource::Buffer staging{device_, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
  staging.write(pixels.get(), imageSize);

  auto imageDesc = desc_.image;
  imageDesc.width = width;
  imageDesc.height = height;
  imageDesc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  image_->update(imageDesc);

  transitionImageLayout(device_, command_pool_, *image_,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(device_, command_pool_, staging, *image_, 1, channels);

  if (desc_.layout != VK_IMAGE_LAYOUT_UNDEFINED &&
      desc_.layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    transitionImageLayout(device_, command_pool_, *image_,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, desc_.layout);
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
