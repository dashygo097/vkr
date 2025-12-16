#pragma once

#include "./instance.hh"
#include "./window.hh"

namespace vkr {

class Surface {
public:
  explicit Surface(const Window &window, const Instance &instance);
  ~Surface();

  Surface(const Surface &) = delete;
  Surface &operator=(const Surface &) = delete;

  [[nodiscard]] VkSurfaceKHR surface() const noexcept { return _surface; }

private:
  // dependencies
  const Instance &instance;
  const Window &window;

  // components
  VkSurfaceKHR _surface{VK_NULL_HANDLE};
};
} // namespace vkr
