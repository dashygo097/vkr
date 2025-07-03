#pragma once

#include "ctx.hpp"

#include <glm/glm.hpp>
#include <vector>

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

class UniformBuffers {
public:
  UniformBuffers(const UniformBufferObject &object, VkDevice device,
                 VkPhysicalDevice physicalDevice);
  UniformBuffers(const UniformBufferObject &object, const VulkanContext &ctx);
  ~UniformBuffers();

  void create();
  void destroy();
  void update(uint32_t currentFrame, const UniformBufferObject &object);
  ;

  std::vector<VkBuffer> getVkBuffers() const { return uniformBuffers; }
  std::vector<VkDeviceMemory> getVkBuffersMemory() const { return memories; }
  std::vector<void *> getMapped() const { return mapped; }

private:
  // dependencies
  VkDevice device;
  VkPhysicalDevice physicalDevice;

  // componets
  UniformBufferObject object;

  std::vector<VkBuffer> uniformBuffers{};
  std::vector<VkDeviceMemory> memories{};
  std::vector<void *> mapped{};
};
