#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include "ctx.hpp"

class Surface {
public:
  Surface(VkInstance instance, GLFWwindow *window);
  Surface(const VulkanContext &ctx);
  ~Surface();

  VkSurfaceKHR getVkSurface() const { return surface; }

private:
  // dependencies
  VkInstance instance;
  GLFWwindow *window;

  // components
  VkSurfaceKHR surface{VK_NULL_HANDLE};
};
