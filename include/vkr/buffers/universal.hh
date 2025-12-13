#pragma once

#include <vulkan/vulkan.h>

namespace vkr {

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties,
                        VkPhysicalDevice physicalDevice);

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory, VkDevice device,
                  VkPhysicalDevice physicalDevice);

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
                VkCommandPool commandPool, VkQueue graphicsQueue,
                VkDevice device);
} // namespace vkr
