#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

class Surface {
public:
  Surface(VkInstance instance, GLFWwindow *window);
  ~Surface();

  VkSurfaceKHR getSurface() const;

private:
  VkInstance instance;
  GLFWwindow *window;
  VkSurfaceKHR surface;
};
