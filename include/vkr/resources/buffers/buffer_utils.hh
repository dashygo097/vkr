#pragma once

#include <vulkan/vulkan.h>

namespace vkr::resource {

auto findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties,
                        VkPhysicalDevice physicalDevice) -> uint32_t;

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory, VkDevice device,
                  VkPhysicalDevice physicalDevice);

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
                VkCommandPool commandPool, VkQueue graphicsQueue,
                VkDevice device);
} // namespace vkr::resource
