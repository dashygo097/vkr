#pragma once

#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/buffer.hh"
#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>

namespace vkr::resource {

struct StorageBufferDesc {
  size_t elementCount{0};
  VkBufferUsageFlags usage{VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT};
  VkMemoryPropertyFlags memoryProperties{VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
  bool mapOnCreate{false};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return elementCount != 0 &&
           (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) != 0;
  }

  [[nodiscard]] static auto hostVisible(size_t elementCount)
      -> StorageBufferDesc {
    StorageBufferDesc desc{};
    desc.elementCount = elementCount;
    desc.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    desc.mapOnCreate = true;
    return desc;
  }

  [[nodiscard]] static auto deviceLocal(size_t elementCount)
      -> StorageBufferDesc {
    StorageBufferDesc desc{};
    desc.elementCount = elementCount;
    desc.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.mapOnCreate = false;
    return desc;
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

    if (elementOffset + elementCount > desc_.elementCount) {
      VKR_RES_ERROR("Storage buffer write exceeds buffer bounds");
    }

    const VkDeviceSize offset =
        static_cast<VkDeviceSize>(sizeof(ElementType) * elementOffset);
    const VkDeviceSize size =
        static_cast<VkDeviceSize>(sizeof(ElementType) * elementCount);
    target_->write(elements, size, offset);
  }

  void read(std::vector<ElementType> &elements, size_t elementOffset = 0) {
    if (elements.empty()) {
      VKR_RES_ERROR("Cannot read empty storage buffer data");
    }

    if (elementOffset + elements.size() > desc_.elementCount) {
      VKR_RES_ERROR("Storage buffer read exceeds buffer bounds");
    }

    const VkDeviceSize offset =
        static_cast<VkDeviceSize>(sizeof(ElementType) * elementOffset);
    const VkDeviceSize size =
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

  [[nodiscard]] auto elementCount() const noexcept -> size_t {
    return desc_.elementCount;
  }

  [[nodiscard]] auto bufferSize() const noexcept -> VkDeviceSize {
    return static_cast<VkDeviceSize>(sizeof(ElementType) * desc_.elementCount);
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

  [[nodiscard]] auto valid() const noexcept -> bool {
    return target_->isValid();
  }

private:
  const core::Device &device_;

  StorageBufferDesc desc_{};
  std::unique_ptr<Buffer> target_;

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
