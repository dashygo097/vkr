#include "vkr/resource/gpu/image.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/buffer.hh"

namespace vkr::resource {

Image::Image(const core::Device &device) : device_(device) {}

Image::Image(const core::Device &device, const ImageDesc &desc)
    : device_(device), desc_(desc) {
  create();
}

Image::~Image() { destroy(); }

void Image::update(const ImageDesc &desc) {
  destroy();
  desc_ = desc;
  create();
}

void Image::create() {
  if (!desc_.isValid()) {
    VKR_RES_ERROR("ImageDesc is invalid");
  }

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.flags = desc_.flags;
  imageInfo.imageType = desc_.type;
  imageInfo.extent.width = desc_.width;
  imageInfo.extent.height = desc_.height;
  imageInfo.extent.depth = desc_.depth;
  imageInfo.mipLevels = desc_.mipLevels;
  imageInfo.arrayLayers = desc_.arrayLayers;
  imageInfo.format = desc_.format;
  imageInfo.tiling = desc_.tiling;
  imageInfo.initialLayout = desc_.layout;
  imageInfo.usage = desc_.usage;
  imageInfo.samples = desc_.samples;
  imageInfo.sharingMode = desc_.sharingMode;

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
      memRequirements.memoryTypeBits, desc_.memoryProperties, device_);

  if (vkAllocateMemory(device_.device(), &allocInfo, nullptr, &vk_memory_) !=
      VK_SUCCESS) {
    vkDestroyImage(device_.device(), vk_image_, nullptr);
    vk_image_ = VK_NULL_HANDLE;
    VKR_RES_ERROR("Failed to allocate image memory");
  }

  if (vkBindImageMemory(device_.device(), vk_image_, vk_memory_, 0) !=
      VK_SUCCESS) {
    destroy();
    VKR_RES_ERROR("Failed to bind image memory");
  }

  layout_ = desc_.layout;
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

  layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
}

} // namespace vkr::resource
