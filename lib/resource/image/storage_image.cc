#include "vkr/resource/image/storage_image.hh"
#include "vkr/logger.hh"

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
    VKR_RES_ERROR("Transfer command pool cannot transition storage image for "
                  "shader access");
  }

  VKR_RES_ERROR("Unsupported command pool role for storage image shader "
                "access");
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
    VKR_RES_ERROR("Failed to allocate storage image command buffer");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    vkFreeCommandBuffers(device.device(), commandPool.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to begin storage image command buffer");
  }

  return commandBuffer;
}

void endSingleTimeCommands(const core::Device &device,
                           const core::CommandPool &commandPool,
                           VkCommandBuffer commandBuffer) {
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    vkFreeCommandBuffers(device.device(), commandPool.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to end storage image command buffer");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(commandPool.queue(), 1, &submitInfo, VK_NULL_HANDLE) !=
      VK_SUCCESS) {
    vkFreeCommandBuffers(device.device(), commandPool.commandPool(), 1,
                         &commandBuffer);
    VKR_RES_ERROR("Failed to submit storage image command buffer");
  }

  vkQueueWaitIdle(commandPool.queue());
  vkFreeCommandBuffers(device.device(), commandPool.commandPool(), 1,
                       &commandBuffer);
}

void transitionImageLayout(const core::Device &device,
                           const core::CommandPool &commandPool, Image &image,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout) {
  if (oldLayout == newLayout) {
    return;
  }

  if (oldLayout != VK_IMAGE_LAYOUT_UNDEFINED ||
      newLayout != VK_IMAGE_LAYOUT_GENERAL) {
    VKR_RES_ERROR("Unsupported storage image layout transition: {} -> {}",
                  static_cast<int>(oldLayout), static_cast<int>(newLayout));
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
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT |
                          VK_ACCESS_SHADER_WRITE_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       shaderStageFor(commandPool), 0, 0, nullptr, 0, nullptr,
                       1, &barrier);

  endSingleTimeCommands(device, commandPool, commandBuffer);
  image.setLayout(newLayout);
}

} // namespace

StorageImage::StorageImage(const core::Device &device,
                           const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool),
      image_(std::make_unique<Image>(device_)),
      image_view_(std::make_unique<ImageView>(device_)) {}

StorageImage::~StorageImage() { destroy(); }

void StorageImage::update(const StorageImageDesc &desc) {
  desc_ = desc;
  create();
}

void StorageImage::create() {
  destroy();

  if (!desc_.isValid()) {
    VKR_RES_ERROR("StorageImageDesc is invalid");
  }

  ImageDesc imageDesc = ImageDesc::storage2D(desc_.width, desc_.height,
                                             desc_.format);
  imageDesc.usage = desc_.usage;
  imageDesc.aspectMask = desc_.aspectMask;
  imageDesc.defaultViewType = desc_.viewType;
  imageDesc.layout = VK_IMAGE_LAYOUT_UNDEFINED;

  image_->update(imageDesc);

  if (desc_.layout != VK_IMAGE_LAYOUT_UNDEFINED) {
    transitionImageLayout(device_, command_pool_, *image_,
                          VK_IMAGE_LAYOUT_UNDEFINED, desc_.layout);
  }

  image_view_->update(ImageViewDesc::fromImage(*image_));
}

void StorageImage::destroy() {
  if (image_view_) {
    image_view_->destroy();
  }

  if (image_) {
    image_->destroy();
  }
}

auto StorageImage::descriptorInfo() const noexcept -> VkDescriptorImageInfo {
  VkDescriptorImageInfo info{};
  info.sampler = VK_NULL_HANDLE;
  info.imageView = imageView();
  info.imageLayout = layout();
  return info;
}

} // namespace vkr::resource
