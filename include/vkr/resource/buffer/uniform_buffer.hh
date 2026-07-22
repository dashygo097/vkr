#pragma once

#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffer/buffer.hh"
#include <cstddef>
#include <cstring>
#include <memory>

namespace vkr::resource {

template <typename UniformType> class UniformBuffer {
public:
  explicit UniformBuffer(const core::Device &device)
      : device_(device), target_(std::make_unique<Buffer>(device)) {
    create();
  }

  ~UniformBuffer() { destroy(); }

  UniformBuffer(const UniformBuffer &) = delete;
  auto operator=(const UniformBuffer &) -> UniformBuffer & = delete;

  UniformBuffer(UniformBuffer &&) = delete;
  auto operator=(UniformBuffer &&) -> UniformBuffer & = delete;

  [[nodiscard]] auto buffer() const noexcept -> VkBuffer {
    return target_->buffer();
  }

  [[nodiscard]] auto memory() const noexcept -> VkDeviceMemory {
    return target_->memory();
  }

  [[nodiscard]] auto byteSize() const noexcept -> VkDeviceSize {
    return sizeof(UniformType);
  }

  [[nodiscard]] auto
  descriptorInfo(VkDeviceSize offset = 0,
                 VkDeviceSize range = sizeof(UniformType)) const noexcept
      -> VkDescriptorBufferInfo {
    VkDescriptorBufferInfo info{};
    info.buffer = buffer();
    info.offset = offset;
    info.range = range;
    return info;
  }

  [[nodiscard]] auto isMapped() const noexcept -> bool {
    return target_->isMapped();
  }

  void updateRaw(const void *data, size_t size) {
    if (size != sizeof(UniformType)) {
      VKR_RES_ERROR("Size mismatch in uniform buffer update!");
    }
    update(*static_cast<const UniformType *>(data));
  }

  void update(const UniformType &newObject) {
    if (!target_->isMapped()) {
      VKR_RES_ERROR("Uniform buffer memory is not mapped!");
    }
    memcpy(target_->mapped(), &newObject, sizeof(UniformType));
  }

protected:
  void create() {

    VkDeviceSize bufferSize = sizeof(UniformType);
    target_->update(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    (void)target_->map(bufferSize);
  }

  void destroy() { target_->destroy(); }

private:
  // dependencies
  const core::Device &device_;

  // components
  std::unique_ptr<Buffer> target_;
};

} // namespace vkr::resource
