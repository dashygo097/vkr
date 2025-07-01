#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

#include "ctx.hpp"

class Window {
public:
  uint32_t width;
  uint32_t height;
  const char *title;

  Window(uint32_t width, uint32_t height, const std::string title);
  Window(const VulkanContext &ctx);
  ~Window();

  bool shouldClose() const;
  void pollEvents() const;

  GLFWwindow *getGLFWWindow() const;

private:
  // components
  GLFWwindow *window{nullptr};
};
