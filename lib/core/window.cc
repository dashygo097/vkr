#include "vkr/core/window.hh"

namespace vkr::core {
Window::Window(std::string title, uint32_t width, uint32_t height)
    : width_(width), height_(height), title_(title) {
  if (!glfwInit()) {
    throw std::runtime_error("[ERROR] Failed to initialize GLFW");
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
  if (!window_) {
    glfwTerminate();
    throw std::runtime_error("[ERROR] Failed to create GLFW window");
  }
}

Window::~Window() {
  if (window_) {
    glfwDestroyWindow(window_);
  }
  glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window_); }
void Window::pollEvents() const { glfwPollEvents(); }
} // namespace vkr::core
