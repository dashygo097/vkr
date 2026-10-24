#include "vkr/resource/buffer/buffer.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <cstddef>
#include <cstring>

namespace vkr::res {

Buffer::Buffer(const core::Device &device) : device_(device) {}

Buffer::~Buffer() { destroy(); }

Buffer::Buffer(Buffer &&other) noexcept
    : device_(other.device_), vk_buffer_(other.vk_buffer_),
      vk_memory_(other.vk_memory_), size_(other.size_),
      allocation_size_(other.allocation_size_), atom_size_(other.atom_size_),
      usage_(other.usage_), memory_properties_(other.memory_properties_),
      mapped_(other.mapped_), mapped_offset_(other.mapped_offset_) {
  other.vk_buffer_ = VK_NULL_HANDLE;
  other.vk_memory_ = VK_NULL_HANDLE;
  other.size_ = 0;
  other.allocation_size_ = 0;
  other.atom_size_ = 1;
  other.usage_ = 0;
  other.memory_properties_ = 0;
  other.mapped_ = nullptr;
  other.mapped_offset_ = 0;
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

  VkPhysicalDeviceMemoryProperties memoryProperties{};
  vkGetPhysicalDeviceMemoryProperties(device_.physicalDevice(),
                                      &memoryProperties);
  memory_properties_ =
      memoryProperties.memoryTypes[allocInfo.memoryTypeIndex].propertyFlags;
  allocation_size_ = allocInfo.allocationSize;
  if (hostVisible() &&
      (memory_properties_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
    VkPhysicalDeviceProperties deviceProperties{};
    vkGetPhysicalDeviceProperties(device_.physicalDevice(), &deviceProperties);
    atom_size_ = deviceProperties.limits.nonCoherentAtomSize;
  }

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
  allocation_size_ = 0;
  atom_size_ = 1;
  usage_ = 0;
  memory_properties_ = 0;
}

auto Buffer::map(VkDeviceSize size, VkDeviceSize offset) -> void * {
  validateRange(size, offset);

  if (!hostVisible()) {
    VKR_RES_ERROR("Cannot map buffer without HOST_VISIBLE memory");
  }

  if (mapped_ == nullptr) {
    if (vkMapMemory(device_.device(), vk_memory_, 0, VK_WHOLE_SIZE, 0,
                    &mapped_) != VK_SUCCESS) {
      VKR_RES_ERROR("Failed to map buffer memory");
    }
    mapped_offset_ = offset;
  }

  return static_cast<std::byte *>(mapped_) + offset;
}

void Buffer::unmap() noexcept {
  if (mapped_ != nullptr) {
    vkUnmapMemory(device_.device(), vk_memory_);
    mapped_ = nullptr;
  }
  mapped_offset_ = 0;
}

void Buffer::write(const void *data, VkDeviceSize size, VkDeviceSize offset) {
  if (data == nullptr || size == 0 || size == VK_WHOLE_SIZE) {
    VKR_RES_ERROR("Buffer write requires data and an explicit nonzero size");
  }

  const bool alreadyMapped = mapped_ != nullptr;
  auto *mapped = map(size, offset);
  std::memcpy(mapped, data, static_cast<size_t>(size));
  flush(size, offset);
  if (!alreadyMapped) {
    unmap();
  }
}

void Buffer::flush(VkDeviceSize size, VkDeviceSize offset) {
  const auto range = mappedRange(size, offset);
  if ((memory_properties_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0) {
    return;
  }
  if (vkFlushMappedMemoryRanges(device_.device(), 1, &range) != VK_SUCCESS) {
    VKR_RES_ERROR("Failed to flush buffer memory");
  }
}

void Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
  const auto range = mappedRange(size, offset);
  if ((memory_properties_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0) {
    return;
  }
  if (vkInvalidateMappedMemoryRanges(device_.device(), 1, &range) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to invalidate buffer memory");
  }
}

void Buffer::validateRange(VkDeviceSize size, VkDeviceSize offset) const {
  if (!isValid()) {
    VKR_RES_ERROR("Cannot access invalid buffer");
  }
  if (offset >= size_ || size == 0 ||
      (size != VK_WHOLE_SIZE && size > size_ - offset)) {
    VKR_RES_ERROR("Buffer access exceeds buffer bounds");
  }
}

auto Buffer::mappedRange(VkDeviceSize size, VkDeviceSize offset) const
    -> VkMappedMemoryRange {
  validateRange(size, offset);
  if (!isMapped()) {
    VKR_RES_ERROR("Cannot synchronize unmapped buffer memory");
  }

  const auto end = size == VK_WHOLE_SIZE ? size_ : offset + size;
  const auto padding = (atom_size_ - end % atom_size_) % atom_size_;
  const auto alignedEnd = end + std::min(padding, allocation_size_ - end);

  VkMappedMemoryRange range{};
  range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  range.memory = vk_memory_;
  range.offset = offset - offset % atom_size_;
  range.size = alignedEnd - range.offset;
  return range;
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

} // namespace vkr::res
