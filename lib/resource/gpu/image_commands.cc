#include "vkr/resource/gpu/image_commands.hh"
#include "vkr/logger.hh"
#include <vector>

namespace vkr::resource {

namespace {

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

} // namespace

void ImageCommands::transitionLayout(const core::Device &device,
                                     const core::CommandPool &commandPool,
                                     Image &image, VkImageLayout oldLayout,
                                     VkImageLayout newLayout) {
  if (oldLayout == newLayout) {
    return;
  }

  VkCommandBuffer commandBuffer =
      beginSingleTimeCommands(device, commandPool);

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
    VKR_RES_ERROR("Unsupported image layout transition: {} -> {}",
                  static_cast<int>(oldLayout), static_cast<int>(newLayout));
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(device, commandPool, commandBuffer);
  image.setLayout(newLayout);
}

void ImageCommands::copyBufferToImage(const core::Device &device,
                                      const core::CommandPool &commandPool,
                                      const Buffer &buffer, Image &image,
                                      uint32_t layers, uint32_t channels) {
  VkCommandBuffer commandBuffer =
      beginSingleTimeCommands(device, commandPool);

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

auto ImageCommands::beginSingleTimeCommands(
    const core::Device &device,
    const core::CommandPool &commandPool) -> VkCommandBuffer {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool.commandPool();
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
  if (vkAllocateCommandBuffers(device.device(), &allocInfo, &commandBuffer) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to allocate image command buffer");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    vkFreeCommandBuffers(device.device(), commandPool.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to begin image command buffer");
  }

  return commandBuffer;
}

void ImageCommands::endSingleTimeCommands(
    const core::Device &device, const core::CommandPool &commandPool,
    VkCommandBuffer commandBuffer) {
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    vkFreeCommandBuffers(device.device(), commandPool.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to end image command buffer");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(commandPool.queue(), 1, &submitInfo, VK_NULL_HANDLE) !=
      VK_SUCCESS) {
    vkFreeCommandBuffers(device.device(), commandPool.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to submit image command buffer");
  }

  vkQueueWaitIdle(commandPool.queue());
  vkFreeCommandBuffers(device.device(), commandPool.commandPool(), 1,
                       &commandBuffer);
}

} // namespace vkr::resource
