#pragma once

#include "ctx.hpp"

class IndexBuffer {
public:
  IndexBuffer(const std::vector<uint16_t> &indices, VkDevice device,
              VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
              VkQueue graphicsQueue);
  IndexBuffer(const std::vector<uint16_t> &indices, const VulkanContext &ctx);
  ~IndexBuffer();

  void create();
  void destroy();
  void update(const std::vector<uint16_t> &indices);

  VkBuffer getVkBuffer() const { return indexBuffer; }
  VkDeviceMemory getVkBufferMemory() const { return memory; }

private:
  // components
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkCommandPool commandPool;
  VkQueue graphicsQueue;

  // dependencies
  std::vector<uint16_t> indices;
  VkBuffer indexBuffer;
  VkDeviceMemory memory;
};
