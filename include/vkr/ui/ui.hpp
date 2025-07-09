#pragma once

#include "../ctx.hpp"
#include "./fps.hpp"

class UI {
public:
  UI(GLFWwindow *window, VkInstance instance, VkSurfaceKHR surface,
     VkPhysicalDevice physicalDevice, VkDevice device, VkRenderPass renderPass,
     VkQueue graphicsQueue, VkDescriptorPool descriptorPool,
     VkCommandPool commandPool);
  UI(const VulkanContext &ctx);
  ~UI();

  UI(const UI &) = delete;
  UI &operator=(const UI &) = delete;

  void render(float fps);

  VkDescriptorPool getVkDescriptorPool() const { return descriptorPool; }

private:
  // dependencies
  GLFWwindow *window;
  VkInstance instance;
  VkSurfaceKHR surface;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkRenderPass renderPass;
  VkQueue graphicsQueue;
  VkDescriptorPool descriptorPool;
  VkCommandPool commandPool;

  // components
  std::unique_ptr<FPSPanel> fps_panel;
};
