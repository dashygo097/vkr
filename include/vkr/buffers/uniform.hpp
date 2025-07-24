#pragma once

#include "../ctx.hpp"

namespace vkr {

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

  UniformBuffers(const UniformBuffers &) = delete;
  UniformBuffers &operator=(const UniformBuffers &) = delete;

  void create();
  void destroy();
  void update(uint32_t currentFrame, const UniformBufferObject &object);
  ;

  [[nodiscard]] std::vector<VkBuffer> buffers() const noexcept {
    return _uniformBuffers;
  }
  [[nodiscard]] std::vector<VkDeviceMemory> buffersMemory() const noexcept {
    return _memories;
  }
  [[nodiscard]] std::vector<void *> mapped() const noexcept { return _mapped; }

private:
  // dependencies
  VkDevice device;
  VkPhysicalDevice physicalDevice;

  // componets
  UniformBufferObject object;

  std::vector<VkBuffer> _uniformBuffers{};
  std::vector<VkDeviceMemory> _memories{};
  std::vector<void *> _mapped{};
};
} // namespace vkr
