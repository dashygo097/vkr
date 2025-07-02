#include "impl/uniform.hpp"
#include "impl/universal_buffer.hpp"
#include "interface/command_buffers.hpp"

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
  VkDeviceSize bufferSize = sizeof(UniformBuffers);

  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  memories.resize(MAX_FRAMES_IN_FLIGHT);
  mapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffers[i], memories[i], device, physicalDevice);

    vkMapMemory(device, memories[i], 0, bufferSize, 0, &mapped[i]);
  }
}

void UniformBuffers::destroy() {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (mapped[i]) {
      vkUnmapMemory(device, memories[i]);
      mapped[i] = nullptr;
    }
    if (memories[i] != VK_NULL_HANDLE) {
      vkFreeMemory(device, memories[i], nullptr);
      memories[i] = VK_NULL_HANDLE;
    }
    if (uniformBuffers[i] != VK_NULL_HANDLE) {
      vkDestroyBuffer(device, uniformBuffers[i], nullptr);
      uniformBuffers[i] = VK_NULL_HANDLE;
    }
  }
  mapped.clear();
  memories.clear();
  uniformBuffers.clear();
}

void UniformBuffers::update(const UniformBufferObject &newObject) {
  object = newObject;
  destroy();
  create();
}
