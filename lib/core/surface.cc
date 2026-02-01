#include "vkr/core/surface.hh"
#include "vkr/logger.hh"

namespace vkr::core {
Surface::Surface(const Instance &instance, const Window &window)
    : instance_(instance), window_(window) {
  VKR_CORE_INFO("Creating window surface...");

  if (glfwCreateWindowSurface(instance_.instance(), window_.glfwWindow(),
                              nullptr, &surface_) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to create window surface!");
  }

  VKR_CORE_INFO("Window surface created.");
}

Surface::~Surface() {
  vkDestroySurfaceKHR(instance_.instance(), surface_, nullptr);
}
} // namespace vkr::core
