#pragma once

#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffer/buffer.hh"
#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>

namespace vkr::resource {

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

  auto reserve(size_t elementCapacity) noexcept -> StorageBufferDesc & {
    capacity = elementCapacity;
    return *this;
  }

  auto elements(size_t elementCapacity) noexcept -> StorageBufferDesc & {
    capacity = elementCapacity;
    return *this;
  }

  auto capacityElements(size_t elementCapacity) noexcept
      -> StorageBufferDesc & {
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
    return desc.reserve(capacity).storage().transfer().hostVisible().mapped();
  }

  [[nodiscard]] static auto deviceLocal(size_t capacity) -> StorageBufferDesc {
    StorageBufferDesc desc{};
    return desc.reserve(capacity).storage().transfer().deviceLocal();
  }

  [[nodiscard]] static auto input(size_t capacity) -> StorageBufferDesc {
    StorageBufferDesc desc{};
    return desc.reserve(capacity)
        .usageFlags(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        .readonly()
        .transfer()
        .hostVisible()
        .mapped();
  }

  [[nodiscard]] static auto output(size_t capacity) -> StorageBufferDesc {
    StorageBufferDesc desc{};
    return desc.reserve(capacity)
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

template <typename ElementType> class StorageBuffer {
public:
  explicit StorageBuffer(const core::Device &device)
      : device_(device), target_(std::make_unique<Buffer>(device)) {}

  StorageBuffer(const core::Device &device, const StorageBufferDesc &desc)
      : device_(device), target_(std::make_unique<Buffer>(device)) {
    update(desc);
  }

  ~StorageBuffer() { destroy(); }

  StorageBuffer(const StorageBuffer &) = delete;
  auto operator=(const StorageBuffer &) -> StorageBuffer & = delete;

  StorageBuffer(StorageBuffer &&) = delete;
  auto operator=(StorageBuffer &&) -> StorageBuffer & = delete;

  void update(const StorageBufferDesc &desc) {
    desc_ = desc;
    create();
  }

  void destroy() { target_->destroy(); }

  void write(const std::vector<ElementType> &elements,
             size_t elementOffset = 0) {
    write(elements.data(), elements.size(), elementOffset);
  }

  void write(const ElementType *elements, size_t elementCount,
             size_t elementOffset = 0) {
    if (elements == nullptr || elementCount == 0) {
      VKR_RES_ERROR("Cannot write empty storage buffer data");
    }

    if (elementOffset + elementCount > desc_.capacity) {
      VKR_RES_ERROR("Storage buffer write exceeds buffer bounds");
    }

    if ((desc_.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
      VKR_RES_ERROR("Cannot write device-local storage buffer directly from "
                    "CPU; use a staging/upload path");
    }

    auto offset =
        static_cast<VkDeviceSize>(sizeof(ElementType) * elementOffset);
    auto size = static_cast<VkDeviceSize>(sizeof(ElementType) * elementCount);
    target_->write(elements, size, offset);
  }

  void read(std::vector<ElementType> &elements, size_t elementOffset = 0) {
    if (elements.empty()) {
      VKR_RES_ERROR("Cannot read empty storage buffer data");
    }

    if (elementOffset + elements.size() > desc_.capacity) {
      VKR_RES_ERROR("Storage buffer read exceeds buffer bounds");
    }

    if ((desc_.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
      VKR_RES_ERROR("Cannot read device-local storage buffer directly from "
                    "CPU; use a staging/download path");
    }

    auto offset =
        static_cast<VkDeviceSize>(sizeof(ElementType) * elementOffset);
    auto size =
        static_cast<VkDeviceSize>(sizeof(ElementType) * elements.size());

    const bool alreadyMapped = target_->isMapped();
    auto *mapped = static_cast<std::byte *>(
        alreadyMapped ? target_->mapped() : target_->map(size, offset));
    if (alreadyMapped) {
      mapped += offset;
    }

    std::memcpy(elements.data(), mapped, static_cast<size_t>(size));

    if (!alreadyMapped) {
      target_->unmap();
    }
  }

  [[nodiscard]] auto desc() const noexcept -> const StorageBufferDesc & {
    return desc_;
  }

  [[nodiscard]] auto buffer() const noexcept -> VkBuffer {
    return target_->buffer();
  }

  [[nodiscard]] auto memory() const noexcept -> VkDeviceMemory {
    return target_->memory();
  }

  [[nodiscard]] auto capacity() const noexcept -> size_t {
    return desc_.capacity;
  }

  [[nodiscard]] auto bufferSize() const noexcept -> VkDeviceSize {
    return static_cast<VkDeviceSize>(sizeof(ElementType) * desc_.capacity);
  }

  [[nodiscard]] auto
  descriptorInfo(VkDeviceSize offset = 0,
                 VkDeviceSize range = VK_WHOLE_SIZE) const noexcept
      -> VkDescriptorBufferInfo {
    VkDescriptorBufferInfo info{};
    info.buffer = buffer();
    info.offset = offset;
    info.range = range;
    return info;
  }

  [[nodiscard]] auto hostVisible() const noexcept -> bool {
    return (desc_.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
  }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return target_->isValid();
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  StorageBufferDesc desc_{};
  std::unique_ptr<Buffer> target_;

  // helpers
  void create() {
    destroy();

    if (!desc_.isValid()) {
      VKR_RES_ERROR("StorageBufferDesc is invalid");
    }

    target_->update(bufferSize(), desc_.usage, desc_.memoryProperties);

    if (desc_.mapOnCreate) {
      (void)target_->map(bufferSize());
    }
  }
};

} // namespace vkr::resource
