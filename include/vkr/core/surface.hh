#pragma once

#include "vkr/core/instance.hh"
#include "vkr/core/window.hh"

namespace vkr::core {

class Surface {
public:
  explicit Surface(const Instance &instance, const Window &window);
  ~Surface();

  Surface(const Surface &) = delete;
  auto operator=(const Surface &) -> Surface & = delete;

  [[nodiscard]] auto surface() const noexcept -> VkSurfaceKHR {
    return surface_;
  }

private:
  // dependencies
  const Instance &instance_;
  const Window &window_;

  // components
  VkSurfaceKHR surface_{VK_NULL_HANDLE};
};
} // namespace vkr::core
