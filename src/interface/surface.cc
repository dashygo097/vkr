#include "vkr/interface/surface.hpp"

namespace vkr {
Surface::Surface(VkInstance instance, GLFWwindow *window)
    : instance(instance), window(window) {
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}

Surface::Surface(const VulkanContext &ctx)
    : Surface(ctx.instance, ctx.window) {}

Surface::~Surface() { vkDestroySurfaceKHR(instance, surface, nullptr); }
} // namespace vkr
