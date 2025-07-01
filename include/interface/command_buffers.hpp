#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#define MAX_FRAMES_IN_FLIGHT 2

class CommandBuffers {
public:
  CommandBuffers(VkDevice device, VkCommandPool commandPool);
  ~CommandBuffers();

private:
  // dependencies
  VkDevice device;
  VkCommandPool commandPool;

  // components
  std::vector<VkCommandBuffer> commandBuffers{};
};
