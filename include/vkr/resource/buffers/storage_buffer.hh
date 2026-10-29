#pragma once

#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/buffer.hh"
#include <cstddef>
#include <limits>
#include <type_traits>
#include <vector>

namespace vkr::res {

enum class StorageBufferAccess {
  ReadOnly,
  WriteOnly,
  ReadWrite,
};

struct StorageBufferDesc {
  size_t capacity{0};
  VkBufferUsageFlags usage{VK_BUFFER_USAGE_STORAGE_BUFFER_BIT};
  VkMemoryPropertyFlags memoryProperties{VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
  StorageBufferAccess access{StorageBufferAccess::ReadWrite};
  bool mapOnCreate{false};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return capacity != 0 && (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) != 0 &&
           (!mapOnCreate ||
            (memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0);
  }

  auto elements(size_t elementCapacity) noexcept -> StorageBufferDesc & {
    capacity = elementCapacity;
    return *this;
  }

  auto storage(bool enabled = true) noexcept -> StorageBufferDesc & {
    setUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, enabled);
    return *this;
  }

  auto transferSrc(bool enabled = true) noexcept -> StorageBufferDesc & {
    setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, enabled);
    return *this;
  }

  auto transferDst(bool enabled = true) noexcept -> StorageBufferDesc & {
    setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT, enabled);
    return *this;
  }

  auto transfer(bool enabled = true) noexcept -> StorageBufferDesc & {
    transferSrc(enabled);
    transferDst(enabled);
    return *this;
  }

  auto usageFlags(VkBufferUsageFlags flags) noexcept -> StorageBufferDesc & {
    usage = flags;
    return *this;
  }

  auto addUsage(VkBufferUsageFlags flags) noexcept -> StorageBufferDesc & {
    usage |= flags;
    return *this;
  }

  auto readonly() noexcept -> StorageBufferDesc & {
    access = StorageBufferAccess::ReadOnly;
    return *this;
  }

  auto writeonly() noexcept -> StorageBufferDesc & {
    access = StorageBufferAccess::WriteOnly;
    return *this;
  }

  auto readwrite() noexcept -> StorageBufferDesc & {
    access = StorageBufferAccess::ReadWrite;
    return *this;
  }

  auto hostVisible(bool coherent = true) noexcept -> StorageBufferDesc & {
    memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    if (coherent) {
      memoryProperties |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    return *this;
  }

  auto hostCoherent(bool enabled = true) noexcept -> StorageBufferDesc & {
    setMemoryProperty(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, enabled);
    return *this;
  }

  auto hostCached(bool enabled = true) noexcept -> StorageBufferDesc & {
    setMemoryProperty(VK_MEMORY_PROPERTY_HOST_CACHED_BIT, enabled);
    return *this;
  }

  auto deviceLocal() noexcept -> StorageBufferDesc & {
    memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    mapOnCreate = false;
    return *this;
  }

  auto memory(VkMemoryPropertyFlags flags) noexcept -> StorageBufferDesc & {
    memoryProperties = flags;
    if ((memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
      mapOnCreate = false;
    }
    return *this;
  }

  auto addMemory(VkMemoryPropertyFlags flags) noexcept -> StorageBufferDesc & {
    memoryProperties |= flags;
    return *this;
  }

  auto mapped(bool enabled = true) noexcept -> StorageBufferDesc & {
    mapOnCreate = enabled;
    if (enabled) {
      memoryProperties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }
    return *this;
  }

  auto unmapped() noexcept -> StorageBufferDesc & {
    mapOnCreate = false;
    return *this;
  }

  [[nodiscard]] static auto hostVisible(size_t capacity) -> StorageBufferDesc {
    StorageBufferDesc desc{};
    return desc.elements(capacity).storage().transfer().hostVisible().mapped();
  }

  [[nodiscard]] static auto deviceLocal(size_t capacity) -> StorageBufferDesc {
    StorageBufferDesc desc{};
    return desc.elements(capacity).storage().transfer().deviceLocal();
  }

  [[nodiscard]] static auto input(size_t capacity) -> StorageBufferDesc {
    StorageBufferDesc desc{};
    return desc.elements(capacity)
        .usageFlags(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        .readonly()
        .transfer()
        .hostVisible()
        .mapped();
  }

  [[nodiscard]] static auto output(size_t capacity) -> StorageBufferDesc {
    StorageBufferDesc desc{};
    return desc.elements(capacity)
        .usageFlags(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        .writeonly()
        .transfer()
        .hostVisible()
        .mapped();
  }

private:
  void setUsage(VkBufferUsageFlags flag, bool enabled) noexcept {
    if (enabled) {
      usage |= flag;
    } else {
      usage &= ~flag;
    }
  }

  void setMemoryProperty(VkMemoryPropertyFlags flag, bool enabled) noexcept {
    if (enabled) {
      memoryProperties |= flag;
    } else {
      memoryProperties &= ~flag;
      if (flag == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        mapOnCreate = false;
      }
    }
  }
};

template <typename ElementType> class StorageBuffer : public Buffer {
  static_assert(std::is_trivially_copyable_v<ElementType>,
                "StorageBuffer elements must be trivially copyable");

public:
  explicit StorageBuffer(const core::Device &device) : Buffer(device) {}

  StorageBuffer(const core::Device &device, const StorageBufferDesc &desc)
      : Buffer(device) {
    update(desc);
  }

  ~StorageBuffer() override = default;

  StorageBuffer(const StorageBuffer &) = delete;
  auto operator=(const StorageBuffer &) -> StorageBuffer & = delete;

  StorageBuffer(StorageBuffer &&) noexcept = default;
  auto operator=(StorageBuffer &&) -> StorageBuffer & = delete;

  void update(const StorageBufferDesc &desc) {
    if (!desc.isValid() ||
        desc.capacity >
            std::numeric_limits<VkDeviceSize>::max() / sizeof(ElementType) ||
        desc.capacity >
            std::numeric_limits<size_t>::max() / sizeof(ElementType)) {
      VKR_RES_ERROR("StorageBufferDesc is invalid or its byte size overflows");
    }
    Buffer::update(static_cast<VkDeviceSize>(desc.capacity) *
                       sizeof(ElementType),
                   desc.usage, desc.memoryProperties, desc.mapOnCreate);
    desc_ = desc;
  }

  void write(const std::vector<ElementType> &elements,
             size_t elementOffset = 0) {
    write(elements.data(), elements.size(), elementOffset);
  }

  void write(const ElementType *elements, size_t elementCount,
             size_t elementOffset = 0) {
    if (elements == nullptr || elementCount == 0) {
      VKR_RES_ERROR("Cannot write empty storage buffer data");
    }

    if (elementOffset > capacity() ||
        elementCount > capacity() - elementOffset) {
      VKR_RES_ERROR("Storage buffer write exceeds buffer bounds");
    }

    auto offset =
        static_cast<VkDeviceSize>(sizeof(ElementType) * elementOffset);
    auto size = static_cast<VkDeviceSize>(sizeof(ElementType) * elementCount);
    Buffer::write(elements, size, offset);
  }

  void read(std::vector<ElementType> &elements, size_t elementOffset = 0) {
    read(elements.data(), elements.size(), elementOffset);
  }

  void read(ElementType *elements, size_t elementCount,
            size_t elementOffset = 0) {
    if (elements == nullptr || elementCount == 0) {
      VKR_RES_ERROR("Cannot read empty storage buffer data");
    }

    if (elementOffset > capacity() ||
        elementCount > capacity() - elementOffset) {
      VKR_RES_ERROR("Storage buffer read exceeds buffer bounds");
    }

    auto offset =
        static_cast<VkDeviceSize>(sizeof(ElementType) * elementOffset);
    auto size = static_cast<VkDeviceSize>(sizeof(ElementType) * elementCount);
    Buffer::read(elements, size, offset);
  }

  [[nodiscard]] auto desc() const noexcept -> const StorageBufferDesc & {
    return desc_;
  }

  [[nodiscard]] auto capacity() const noexcept -> size_t {
    return static_cast<size_t>(size() / sizeof(ElementType));
  }

  [[nodiscard]] auto descriptorInfo() const -> VkDescriptorBufferInfo {
    return Buffer::slice(0, size()).descriptorInfo();
  }

  [[nodiscard]] auto slice(size_t elementOffset,
                           size_t elementCount) const & -> BufferSlice {
    if (elementOffset > capacity() ||
        elementCount > capacity() - elementOffset) {
      VKR_RES_ERROR("Storage buffer slice exceeds buffer bounds");
    }
    return Buffer::slice(
        static_cast<VkDeviceSize>(elementOffset) * sizeof(ElementType),
        static_cast<VkDeviceSize>(elementCount) * sizeof(ElementType));
  }
  [[nodiscard]] auto slice(size_t, size_t) const && -> BufferSlice = delete;

private:
  // components
  StorageBufferDesc desc_{};
};

} // namespace vkr::res
