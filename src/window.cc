#include "window.hpp"
#include "GLFW/glfw3.h"
#include <stdexcept>

Window::Window(uint32_t width, uint32_t height, const char *title = "Vulkan") {
  this->width = width;
  this->height = height;
  this->title = title;
  if (!glfwInit()) {
    throw std::runtime_error("[ERROR] Failed to initialize GLFW");
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    throw std::runtime_error("[ERROR] Failed to create GLFW window");
  }
}

Window::~Window() {
  if (window) {
    glfwDestroyWindow(window);
  }
  glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window); }
void Window::pollEvents() const { glfwPollEvents(); }
GLFWwindow *Window::getGLFWWindow() const { return window; }
