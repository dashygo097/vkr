#include "vkr/core/vulkan/surface.hh"

namespace vkr {
Surface::Surface(VkInstance instance, GLFWwindow *window)
    : instance(instance), window(window) {
  if (glfwCreateWindowSurface(instance, window, nullptr, &_surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}

Surface::Surface(const VulkanContext &ctx)
    : Surface(ctx.instance, ctx.window) {}

Surface::~Surface() { vkDestroySurfaceKHR(instance, _surface, nullptr); }
} // namespace vkr
