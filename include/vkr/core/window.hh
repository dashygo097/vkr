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

  [[nodiscard]] GLFWwindow *glfwWindow() const noexcept { return _window; }
  [[nodiscard]] uint32_t width() const noexcept { return _width; }
  [[nodiscard]] uint32_t height() const noexcept { return _height; }
  [[nodiscard]] std::string title() const noexcept { return _title; }

private:
  // components
  GLFWwindow *_window{nullptr};
  uint32_t _width;
  uint32_t _height;
  std::string _title;
};
} // namespace vkr::core
