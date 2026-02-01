#include "vkr/core/surface.hh"

namespace vkr::core {
Surface::Surface(const Instance &instance, const Window &window)
    : instance_(instance), window_(window) {
  if (glfwCreateWindowSurface(instance_.instance(), window_.glfwWindow(),
                              nullptr, &surface_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}

Surface::~Surface() {
  vkDestroySurfaceKHR(instance_.instance(), surface_, nullptr);
}
} // namespace vkr::core
