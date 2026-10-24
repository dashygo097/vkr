#pragma once

#include "vkr/core/device.hh"
#include <cstddef>
#include <vulkan/vulkan.h>

namespace vkr::res {

class Buffer {
public:
  explicit Buffer(const core::Device &device);
  ~Buffer();

  Buffer(const Buffer &) = delete;
  auto operator=(const Buffer &) -> Buffer & = delete;

  Buffer(Buffer &&other) noexcept;
  auto operator=(Buffer &&other) -> Buffer & = delete;

  void update(VkDeviceSize size, VkBufferUsageFlags usage,
              VkMemoryPropertyFlags properties);
  void destroy() noexcept;

  [[nodiscard]] auto buffer() const noexcept -> const VkBuffer & {
    return vk_buffer_;
  }
  [[nodiscard]] auto memory() const noexcept -> const VkDeviceMemory & {
    return vk_memory_;
  }
  [[nodiscard]] auto mapped() const noexcept -> void * {
    return mapped_ ? static_cast<std::byte *>(mapped_) + mapped_offset_
                   : nullptr;
  }
  [[nodiscard]] auto size() const noexcept -> VkDeviceSize { return size_; }
  [[nodiscard]] auto usage() const noexcept -> VkBufferUsageFlags {
    return usage_;
  }
  [[nodiscard]] auto memoryProperties() const noexcept
      -> VkMemoryPropertyFlags {
    return memory_properties_;
  }
  [[nodiscard]] auto isValid() const noexcept -> bool {
    return vk_buffer_ != VK_NULL_HANDLE && vk_memory_ != VK_NULL_HANDLE;
  }
  [[nodiscard]] auto isMapped() const noexcept -> bool {
    return mapped_ != nullptr;
  }
  [[nodiscard]] auto hostVisible() const noexcept -> bool {
    return (memory_properties_ & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
  }

  // Map the allocation and return the requested range. mapped() retains the
  // first mapped view until unmap(); subsequent map() calls honor their offset.
  [[nodiscard]] auto map(VkDeviceSize size = VK_WHOLE_SIZE,
                         VkDeviceSize offset = 0) -> void *;
  void unmap() noexcept;
  void write(const void *data, VkDeviceSize size, VkDeviceSize offset = 0);
  void flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

  static auto findMemoryType(uint32_t typeFilter,
                             VkMemoryPropertyFlags properties,
                             const core::Device &device) -> uint32_t;

private:
  // dependencies
  const core::Device &device_;

  // components
  VkBuffer vk_buffer_{VK_NULL_HANDLE};
  VkDeviceMemory vk_memory_{VK_NULL_HANDLE};
  VkDeviceSize size_{0};
  VkDeviceSize allocation_size_{0};
  VkDeviceSize atom_size_{1};
  VkBufferUsageFlags usage_{0};
  VkMemoryPropertyFlags memory_properties_{0};
  void *mapped_{nullptr};
  VkDeviceSize mapped_offset_{0};

  void create(VkDeviceSize size, VkBufferUsageFlags usage,
              VkMemoryPropertyFlags properties);
  void validateRange(VkDeviceSize size, VkDeviceSize offset) const;
  [[nodiscard]] auto mappedRange(VkDeviceSize size, VkDeviceSize offset) const
      -> VkMappedMemoryRange;
};

} // namespace vkr::res
