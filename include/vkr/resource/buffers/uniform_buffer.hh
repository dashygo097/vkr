#pragma once

#include "vkr/core/device.hh"
#include "vkr/resource/buffers/buffer.hh"
#include <cstddef>
#include <type_traits>

namespace vkr::res {

class UniformBuffer : public Buffer {
public:
  explicit UniformBuffer(const core::Device &device, VkDeviceSize size);

  ~UniformBuffer() override = default;

  UniformBuffer(const UniformBuffer &) = delete;
  auto operator=(const UniformBuffer &) -> UniformBuffer & = delete;

  UniformBuffer(UniformBuffer &&) noexcept = default;
  auto operator=(UniformBuffer &&) -> UniformBuffer & = delete;

  [[nodiscard]] auto descriptorInfo() const -> VkDescriptorBufferInfo {
    return Buffer::slice(0, size()).descriptorInfo();
  }

  template <typename T> void update(const T &object) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "UniformBuffer data must be trivially copyable");
    update(&object, sizeof(T));
  }

private:
  void update(const void *data, VkDeviceSize size);
};

} // namespace vkr::res
