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
  auto operator=(const Window &) -> Window & = delete;

  [[nodiscard]] auto shouldClose() const -> bool;
  void pollEvents() const;

  [[nodiscard]] auto glfwWindow() const noexcept -> GLFWwindow * { return window_; }
  [[nodiscard]] auto width() const noexcept -> uint32_t { return width_; }
  [[nodiscard]] auto height() const noexcept -> uint32_t { return height_; }
  [[nodiscard]] auto title() const noexcept -> std::string { return title_; }

private:
  // components
  GLFWwindow *window_{nullptr};
  uint32_t width_;
  uint32_t height_;
  std::string title_;
};
} // namespace vkr::core
