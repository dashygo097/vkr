#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

class Surface {
public:
  Surface(VkInstance instance, GLFWwindow *window);
  ~Surface();

  VkSurfaceKHR getSurface() const { return surface; }

private:
  // dependencies
  VkInstance instance;
  GLFWwindow *window;

  // components
  VkSurfaceKHR surface{VK_NULL_HANDLE};
};
