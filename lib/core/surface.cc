#include "vkr/core/surface.hh"

namespace vkr {
Surface::Surface(const Window &window, const Instance &instance)
    : instance(instance), window(window) {
  if (glfwCreateWindowSurface(instance.instance(), window.glfwWindow(), nullptr,
                              &_surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}

Surface::~Surface() {
  vkDestroySurfaceKHR(instance.instance(), _surface, nullptr);
}
} // namespace vkr
