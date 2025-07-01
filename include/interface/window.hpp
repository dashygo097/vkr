#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window {
public:
  uint32_t width;
  uint32_t height;
  const char *title;

  Window(uint32_t width, uint32_t height, const char *title);
  ~Window();

  bool shouldClose() const;
  void pollEvents() const;

  GLFWwindow *getGLFWWindow() const;

private:
  // components
  GLFWwindow *window;
};
