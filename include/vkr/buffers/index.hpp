#pragma once

#include "../ctx.hpp"

namespace vkr {

class IndexBuffer {
public:
  IndexBuffer(const std::vector<uint16_t> &indices, VkDevice device,
              VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
              VkQueue graphicsQueue);
  IndexBuffer(const std::vector<uint16_t> &indices, const VulkanContext &ctx);
  ~IndexBuffer();

  IndexBuffer(const IndexBuffer &) = delete;
  IndexBuffer &operator=(const IndexBuffer &) = delete;

  IndexBuffer(IndexBuffer &&other) noexcept;
  IndexBuffer &operator=(IndexBuffer &&other) noexcept;

  void create();
  void destroy();
  void update(const std::vector<uint16_t> &indices);

  std::vector<uint16_t> getIndices() const { return indices; }
  VkBuffer getVkBuffer() const { return indexBuffer; }
  VkDeviceMemory getVkBufferMemory() const { return memory; }

private:
  // components
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkCommandPool commandPool;
  VkQueue graphicsQueue;

  // dependencies
  std::vector<uint16_t> indices{};
  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
};
} // namespace vkr
