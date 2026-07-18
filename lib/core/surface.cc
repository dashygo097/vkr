#include "vkr/core/surface.hh"
#include "vkr/logger.hh"

namespace vkr::core {
Surface::Surface(const Instance &instance, const Window &window)
    : instance_(instance), window_(window) {
  VKR_CORE_INFO("Creating window surface...");

  create();

  VKR_CORE_INFO("Window surface created.");
}

Surface::~Surface() {
  if (surface_ != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance_.instance(), surface_, nullptr);
    surface_ = VK_NULL_HANDLE;
  }
}

void Surface::create() {
  switch (instance_.surfaceIntegration()) {
  case SurfaceIntegration::None:
    VKR_CORE_ERROR("Cannot create a surface when surface integration is None");
    break;
  case SurfaceIntegration::GLFW:
    if (glfwCreateWindowSurface(instance_.instance(), window_.glfwWindow(),
                                nullptr, &surface_) != VK_SUCCESS) {
      VKR_CORE_ERROR("Failed to create window surface!");
    }
    break;
  }
}

} // namespace vkr::core
