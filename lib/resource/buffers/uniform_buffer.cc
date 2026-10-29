#include "vkr/resource/buffers/uniform_buffer.hh"
#include "vkr/logger.hh"

namespace vkr::res {

UniformBuffer::UniformBuffer(const core::Device &device, VkDeviceSize size)
    : Buffer(device) {
  if (size == 0 || size > device.properties().limits.maxUniformBufferRange) {
    VKR_RES_ERROR("Uniform buffer size exceeds device limits or is zero");
  }
  Buffer::update(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 true);
}

void UniformBuffer::update(const void *data, VkDeviceSize size) {
  if (size != this->size()) {
    VKR_RES_ERROR("Uniform data size {} does not match buffer size {}", size,
                  this->size());
  }
  Buffer::write(data, size);
}

} // namespace vkr::res
