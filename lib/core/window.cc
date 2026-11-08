#include "vkr/core/window.hh"
#include "vkr/logger.hh"
#include <algorithm>

namespace vkr::core {

std::unordered_map<GLFWwindow *, Window *> Window::windows_{};

Window::Window(WindowDesc &desc) : desc_(desc) {
  VKR_CORE_INFO("Creating window: {} ({}x{})...", desc_.title, desc_.width,
                desc_.height);
  const bool firstWindow = windows_.empty();
  if (firstWindow && !glfwInit()) {
    VKR_CORE_ERROR("Failed to initialize GLFW!");
  }

  try {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, desc_.resizable ? GLFW_TRUE : GLFW_FALSE);

    window_ = glfwCreateWindow(desc_.width, desc_.height, desc_.title.c_str(),
                               nullptr, nullptr);
    if (!window_) {
      VKR_CORE_ERROR("Failed to create GLFW window!");
    }

    windows_.emplace(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);
    VKR_CORE_INFO("Window created successfully.");
  } catch (...) {
    if (window_) {
      glfwSetFramebufferSizeCallback(window_, nullptr);
      windows_.erase(window_);
      glfwDestroyWindow(window_);
      window_ = nullptr;
    }
    if (firstWindow) {
      glfwTerminate();
    }
    throw;
  }
}

Window::~Window() {
  if (!window_) {
    return;
  }

  glfwSetFramebufferSizeCallback(window_, nullptr);
  windows_.erase(window_);
  glfwDestroyWindow(window_);
  window_ = nullptr;
  if (windows_.empty()) {
    glfwTerminate();
  }
}

auto Window::shouldClose() const -> bool {
  return glfwWindowShouldClose(window_);
}

void Window::pollEvents() const { glfwPollEvents(); }

void Window::waitForFramebufferSize() {
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window_, &width, &height);

  while (!shouldClose() && (width == 0 || height == 0)) {
    glfwWaitEvents();
    glfwGetFramebufferSize(window_, &width, &height);
  }

  handleFramebufferResize(width, height);
}

void Window::handleFramebufferResize(int width, int height) {
  desc_.width = static_cast<uint32_t>(std::max(width, 0));
  desc_.height = static_cast<uint32_t>(std::max(height, 0));
  framebuffer_resized_ = true;
}

auto Window::window(GLFWwindow *glfwWindow) noexcept -> Window * {
  const auto it = windows_.find(glfwWindow);
  if (it == windows_.end()) {
    return nullptr;
  }

  return it->second;
}

void Window::framebufferSizeCallback(GLFWwindow *glfwWindow, int width,
                                     int height) {
  auto *window = Window::window(glfwWindow);
  if (!window) {
    return;
  }

  window->handleFramebufferResize(width, height);
}

} // namespace vkr::core
