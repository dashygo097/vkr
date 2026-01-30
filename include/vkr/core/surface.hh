#pragma once

#include "./instance.hh"
#include "./window.hh"

namespace vkr::core {

class Surface {
public:
  explicit Surface(const Window &window, const Instance &instance);
  ~Surface();

  Surface(const Surface &) = delete;
  Surface &operator=(const Surface &) = delete;

  [[nodiscard]] VkSurfaceKHR surface() const noexcept { return surface_; }

private:
  // dependencies
  const Instance &instance_;
  const Window &window_;

  // components
  VkSurfaceKHR surface_{VK_NULL_HANDLE};
};
} // namespace vkr::core
