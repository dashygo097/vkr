#include "vkr/resource/buffer/buffer.hh"
#include "vkr/logger.hh"
#include <cstddef>
#include <cstring>

namespace vkr::resource {

Buffer::Buffer(const core::Device &device) : device_(device) {}

Buffer::Buffer(const core::Device &device, VkDeviceSize size,
               VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    : device_(device) {
  create(size, usage, properties);
}

Buffer::~Buffer() { destroy(); }

Buffer::Buffer(Buffer &&other) noexcept
    : device_(other.device_), vk_buffer_(other.vk_buffer_),
      vk_memory_(other.vk_memory_), size_(other.size_), usage_(other.usage_),
      memory_properties_(other.memory_properties_), mapped_(other.mapped_) {
  other.vk_buffer_ = VK_NULL_HANDLE;
  other.vk_memory_ = VK_NULL_HANDLE;
  other.size_ = 0;
  other.usage_ = 0;
  other.memory_properties_ = 0;
  other.mapped_ = nullptr;
}

void Buffer::create(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties) {
  if (size == 0) {
    VKR_RES_ERROR("Cannot create buffer with zero size");
  }

  size_ = size;
  usage_ = usage;
  memory_properties_ = properties;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size_;
  bufferInfo.usage = usage_;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device_.device(), &bufferInfo, nullptr, &vk_buffer_) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to create buffer");
  }

  VkMemoryRequirements memRequirements{};
  vkGetBufferMemoryRequirements(device_.device(), vk_buffer_, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                             memory_properties_, device_);

  if (vkAllocateMemory(device_.device(), &allocInfo, nullptr, &vk_memory_) !=
      VK_SUCCESS) {
    vkDestroyBuffer(device_.device(), vk_buffer_, nullptr);
    vk_buffer_ = VK_NULL_HANDLE;
    VKR_RES_ERROR("Failed to allocate buffer memory");
  }

  if (vkBindBufferMemory(device_.device(), vk_buffer_, vk_memory_, 0) !=
      VK_SUCCESS) {
    destroy();
    VKR_RES_ERROR("Failed to bind buffer memory");
  }
}

void Buffer::update(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties) {
  destroy();
  create(size, usage, properties);
}

void Buffer::destroy() noexcept {
  unmap();

  if (vk_buffer_ != VK_NULL_HANDLE) {
    vkDestroyBuffer(device_.device(), vk_buffer_, nullptr);
    vk_buffer_ = VK_NULL_HANDLE;
  }

  if (vk_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(device_.device(), vk_memory_, nullptr);
    vk_memory_ = VK_NULL_HANDLE;
  }

  size_ = 0;
  usage_ = 0;
  memory_properties_ = 0;
}

auto Buffer::map(VkDeviceSize size, VkDeviceSize offset) -> void * {
  if (!isValid()) {
    VKR_RES_ERROR("Cannot map invalid buffer");
  }

  if (!hostVisible()) {
    VKR_RES_ERROR("Cannot map buffer without HOST_VISIBLE memory");
  }

  if (mapped_ != nullptr) {
    return mapped_;
  }

  if (vkMapMemory(device_.device(), vk_memory_, offset, size, 0, &mapped_) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to map buffer memory");
  }

  return mapped_;
}

void Buffer::unmap() noexcept {
  if (mapped_ != nullptr) {
    vkUnmapMemory(device_.device(), vk_memory_);
    mapped_ = nullptr;
  }
}

void Buffer::write(const void *data, VkDeviceSize size, VkDeviceSize offset) {
  if (data == nullptr || size == 0) {
    VKR_RES_ERROR("Cannot write empty buffer data");
  }

  const bool alreadyMapped = mapped_ != nullptr;
  auto *mapped = alreadyMapped ? static_cast<std::byte *>(mapped_) + offset
                               : static_cast<std::byte *>(map(size, offset));
  std::memcpy(mapped, data, static_cast<size_t>(size));
  if (!alreadyMapped) {
    unmap();
  }
}

auto Buffer::findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties,
                            const core::Device &device) -> uint32_t {
  VkPhysicalDeviceMemoryProperties memProperties{};
  vkGetPhysicalDeviceMemoryProperties(device.physicalDevice(), &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  VKR_RES_ERROR("Failed to find suitable memory type");
}

} // namespace vkr::resource
