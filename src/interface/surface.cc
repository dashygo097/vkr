#include "interface/surface.hpp"
#include <stdexcept>

Surface::Surface(VkInstance instance, GLFWwindow *window)
    : instance(instance), window(window) {
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}

Surface::~Surface() { vkDestroySurfaceKHR(instance, surface, nullptr); }

VkSurfaceKHR Surface::getSurface() const { return surface; }
