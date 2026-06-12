#include "vkr/core/window.hh"
#include "vkr/logger.hh"

namespace vkr::core {

Window::Window(WindowDesc &desc) : desc_(desc) {
  VKR_CORE_INFO("Creating window: {} ({}x{})...", desc_.title, desc_.width,
                desc_.height);
  if (!glfwInit()) {
    VKR_CORE_ERROR("Failed to initialize GLFW!");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window_ = glfwCreateWindow(desc_.width, desc_.height, desc_.title.c_str(),
                             nullptr, nullptr);
  if (!window_) {
    glfwTerminate();
    VKR_CORE_ERROR("Failed to create GLFW window!");
  }
  VKR_CORE_INFO("Window created successfully.");
}

Window::~Window() {
  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }

  glfwTerminate();
}

auto Window::shouldClose() const -> bool {
  return glfwWindowShouldClose(window_);
}

void Window::pollEvents() const { glfwPollEvents(); }

} // namespace vkr::core
