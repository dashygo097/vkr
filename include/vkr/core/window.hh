#pragma once

#include <GLFW/glfw3.h>
#include <cstdint>
#include <string>

namespace vkr::core {

class Window {
public:
  explicit Window(std::string title, uint32_t width, uint32_t height);
  ~Window();

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  [[nodiscard]] bool shouldClose() const;
  void pollEvents() const;

  [[nodiscard]] GLFWwindow *glfwWindow() const noexcept { return window_; }
  [[nodiscard]] uint32_t width() const noexcept { return width_; }
  [[nodiscard]] uint32_t height() const noexcept { return height_; }
  [[nodiscard]] std::string title() const noexcept { return title_; }

private:
  // components
  GLFWwindow *window_{nullptr};
  uint32_t width_;
  uint32_t height_;
  std::string title_;
};
} // namespace vkr::core
