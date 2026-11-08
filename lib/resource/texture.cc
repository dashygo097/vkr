#include "vkr/resource/texture.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffer.hh"
#include <limits>

namespace vkr::res {

Texture::Texture(const core::Device &device)
    : image_(device), image_view_(device) {}

void Texture::destroy() noexcept {
  image_view_.destroy();
  image_.destroy();
}

void Texture::create(const ImageDesc &desc,
                     const std::optional<ImageViewDesc> &view) {
  if (!desc.isValid() || desc.layout != VK_IMAGE_LAYOUT_UNDEFINED) {
    VKR_RES_ERROR("Texture creation requires a valid image with UNDEFINED "
                  "initial layout");
  }
  Texture::destroy();
  try {
    image_.update(desc);
    auto viewDesc = view ? *view : ImageViewDesc::fromImage(image_);
    viewDesc.image = image();
    image_view_.update(viewDesc);
  } catch (...) {
    Texture::destroy();
    throw;
  }
}

void Texture::validateQueue(const core::CommandPool &commandPool,
                            VkImageLayout target) const {
  if (!valid()) {
    VKR_RES_ERROR("Cannot initialize an uncreated texture");
  }
  if (commandPool.queueRole() == core::CommandQueueRole::Transfer &&
      target != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
      target != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
    VKR_RES_ERROR("Texture shader/attachment initialization requires a "
                  "graphics or compute queue");
  }
  if (commandPool.queueRole() != core::CommandQueueRole::Graphics &&
      (target == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ||
       target == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
       target == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)) {
    VKR_RES_ERROR(
        "Texture attachment initialization requires a graphics queue");
  }
}

void Texture::recordBarrier(const core::CommandBuffer &commands,
                            VkImageLayout source,
                            VkImageLayout destination) const {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = source;
  barrier.newLayout = destination;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image();
  barrier.subresourceRange = {image_.desc().aspectMask, 0,
                              image_.desc().mipLevels, 0,
                              image_.desc().arrayLayers};

  VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  if (source == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  } else if (source != VK_IMAGE_LAYOUT_UNDEFINED) {
    VKR_RES_ERROR("Texture initialization cannot replace an existing layout");
  }

  VkImageUsageFlags requiredUsage{};
  VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  switch (destination) {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    requiredUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    requiredUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    requiredUsage = VK_IMAGE_USAGE_SAMPLED_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  case VK_IMAGE_LAYOUT_GENERAL:
    requiredUsage = VK_IMAGE_USAGE_STORAGE_BIT;
    barrier.dstAccessMask =
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    requiredUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    requiredUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                       VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
    requiredUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    barrier.dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    VKR_RES_ERROR("Unsupported texture initialization layout: {}",
                  static_cast<int>(destination));
  }
  if ((usage() & requiredUsage) == 0) {
    VKR_RES_ERROR("Texture usage does not support layout {}",
                  static_cast<int>(destination));
  }
  vkCmdPipelineBarrier(commands.buffer(), sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);
}

void Texture::transition(const core::CommandPool &commandPool,
                         VkImageLayout target) {
  if (target == layout()) {
    return;
  }
  validateQueue(commandPool, target);
  auto commands = commandPool.allocate();
  commands.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  recordBarrier(commands, layout(), target);
  commands.end();
  commands.submitAndWait();
  image_.layout(target);
}

void Texture::upload(const core::CommandPool &commandPool, const Buffer &buffer,
                     uint32_t channels, VkImageLayout target) {
  if (target == VK_IMAGE_LAYOUT_UNDEFINED) {
    target = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  }
  validateQueue(commandPool, target);
  const auto &desc = image_.desc();
  if (layout() != VK_IMAGE_LAYOUT_UNDEFINED || desc.type != VK_IMAGE_TYPE_2D ||
      desc.mipLevels != 1 || desc.depth != 1 ||
      desc.samples != VK_SAMPLE_COUNT_1_BIT ||
      desc.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT ||
      (usage() & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0 ||
      (buffer.usage() & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0 ||
      !buffer.isValid()) {
    VKR_RES_ERROR("Texture upload requires a new single-mip 2D color image and "
                  "a transfer source buffer");
  }
  uint32_t formatChannels{};
  switch (format()) {
  case VK_FORMAT_R8_UNORM:
  case VK_FORMAT_R8_SRGB:
    formatChannels = 1;
    break;
  case VK_FORMAT_R8G8_UNORM:
  case VK_FORMAT_R8G8_SRGB:
    formatChannels = 2;
    break;
  case VK_FORMAT_R8G8B8_UNORM:
  case VK_FORMAT_R8G8B8_SRGB:
    formatChannels = 3;
    break;
  case VK_FORMAT_R8G8B8A8_UNORM:
  case VK_FORMAT_R8G8B8A8_SRGB:
    formatChannels = 4;
    break;
  default:
    VKR_RES_ERROR(
        "Decoded image upload requires an 8-bit UNORM or SRGB format");
  }
  if (channels != formatChannels) {
    VKR_RES_ERROR("Decoded image channels do not match the texture format");
  }
  const VkDeviceSize pixels = VkDeviceSize{desc.width} * desc.height;
  const VkDeviceSize layerBytes = VkDeviceSize{channels} * desc.arrayLayers;
  if (pixels > std::numeric_limits<VkDeviceSize>::max() / layerBytes ||
      buffer.size() != pixels * layerBytes) {
    VKR_RES_ERROR("Texture upload data size does not match image dimensions");
  }

  auto commands = commandPool.allocate();
  commands.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  recordBarrier(commands, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  VkBufferImageCopy region{};
  region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, desc.arrayLayers};
  region.imageExtent = {desc.width, desc.height, 1};
  vkCmdCopyBufferToImage(commands.buffer(), buffer.buffer(), image(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  if (target != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    recordBarrier(commands, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, target);
  }
  commands.end();
  commands.submitAndWait();
  image_.layout(target);
}

} // namespace vkr::res
