#pragma once

#include "../ctx.hpp"
#include "./fps.hpp"

class UI {
public:
  UI(GLFWwindow *window, VkInstance instance, VkPhysicalDevice physicalDevice,
     VkDevice device, VkRenderPass renderPass, VkQueue graphicsQueue);
  UI(const VulkanContext &ctx);
  ~UI() = default;
  UI(const UI &) = delete;
  UI &operator=(const UI &) = delete;

  void draw(float fps);

private:
  // dependencies
  GLFWwindow *window;
  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkRenderPass renderPass;
  VkQueue graphicsQueue;

  // components
  std::unique_ptr<FPSPanel> fps_panel;
};
