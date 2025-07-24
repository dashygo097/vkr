#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../ctx.hpp"

namespace vkr {

class Surface {
public:
  Surface(VkInstance instance, GLFWwindow *window);
  Surface(const VulkanContext &ctx);
  ~Surface();

  Surface(const Surface &) = delete;
  Surface &operator=(const Surface &) = delete;

  [[nodiscard]] VkSurfaceKHR surface() const noexcept { return _surface; }

private:
  // dependencies
  VkInstance instance{VK_NULL_HANDLE};
  GLFWwindow *window{nullptr};

  // components
  VkSurfaceKHR _surface{VK_NULL_HANDLE};
};
} // namespace vkr
