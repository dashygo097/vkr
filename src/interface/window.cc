#include <stdexcept>

#include "vkr/interface/window.hpp"

namespace vkr {
Window::Window(uint32_t width, uint32_t height, std::string_view title)
    : _width(width), _height(height), _title(title) {
  if (!glfwInit()) {
    throw std::runtime_error("[ERROR] Failed to initialize GLFW");
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  _window = glfwCreateWindow(_width, _height, _title.c_str(), nullptr, nullptr);
  if (!_window) {
    glfwTerminate();
    throw std::runtime_error("[ERROR] Failed to create GLFW window");
  }
}

Window::Window(const VulkanContext &ctx)
    : Window(ctx.width, ctx.height, ctx.title) {}

Window::~Window() {
  if (_window) {
    glfwDestroyWindow(_window);
  }
  glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(_window); }
void Window::pollEvents() const { glfwPollEvents(); }
} // namespace vkr
