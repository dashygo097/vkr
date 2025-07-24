#include "vkr/buffers/uniform.hpp"
#include "vkr/buffers/command.hpp"
#include "vkr/buffers/universal.hpp"

namespace vkr {

UniformBuffers::UniformBuffers(const UniformBufferObject &object,
                               VkDevice device, VkPhysicalDevice physicalDevice)
    : object(object), device(device), physicalDevice(physicalDevice) {
  create();
}

UniformBuffers::UniformBuffers(const UniformBufferObject &object,
                               const VulkanContext &ctx)
    : UniformBuffers(object, ctx.device, ctx.physicalDevice) {}

UniformBuffers::~UniformBuffers() { destroy(); }

void UniformBuffers::create() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  _uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  _memories.resize(MAX_FRAMES_IN_FLIGHT);
  _mapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 _uniformBuffers[i], _memories[i], device, physicalDevice);

    vkMapMemory(device, _memories[i], 0, bufferSize, 0, &_mapped[i]);
  }
}

void UniformBuffers::destroy() {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (_mapped[i]) {
      vkUnmapMemory(device, _memories[i]);
      _mapped[i] = nullptr;
    }
    if (_memories[i] != VK_NULL_HANDLE) {
      vkFreeMemory(device, _memories[i], nullptr);
      _memories[i] = VK_NULL_HANDLE;
    }
    if (_uniformBuffers[i] != VK_NULL_HANDLE) {
      vkDestroyBuffer(device, _uniformBuffers[i], nullptr);
      _uniformBuffers[i] = VK_NULL_HANDLE;
    }
  }
  _mapped.clear();
  _memories.clear();
  _uniformBuffers.clear();
}

void UniformBuffers::update(uint32_t currentFrame,
                            const UniformBufferObject &object) {
  if (currentFrame >= MAX_FRAMES_IN_FLIGHT) {
    throw std::out_of_range("currentFrame exceeds MAX_FRAMES_IN_FLIGHT");
  }
  if (_mapped[currentFrame] == nullptr) {
    throw std::runtime_error("Mapped memory is null for current frame");
  }
  memcpy(_mapped[currentFrame], &object, sizeof(UniformBufferObject));
}
} // namespace vkr
