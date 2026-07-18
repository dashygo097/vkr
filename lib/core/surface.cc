#include "vkr/core/surface.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <string>

namespace vkr::core {
void Surface::appendRequiredInstanceExtensions(InstanceDesc &desc) {
  uint32_t extensionCount = 0;
  const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

  for (uint32_t i = 0; i < extensionCount; ++i) {
    const std::string extension{extensions[i]};
    if (std::find(desc.requiredExtensions.begin(),
                  desc.requiredExtensions.end(),
                  extension) == desc.requiredExtensions.end()) {
      desc.requiredExtensions.push_back(extension);
    }
  }
}

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
