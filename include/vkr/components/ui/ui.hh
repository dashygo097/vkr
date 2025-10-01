#pragma once

#include "../../ctx.hh"
#include "./fps_panel.hh"

namespace vkr {
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

  void render(VkCommandBuffer commandBuffer);

  bool isVisible() const noexcept { return _visible; }

  void visible() { _visible = true; }
  void invisible() { _visible = false; }
  void toggleVisibility() { _visible = !_visible; }

private:
  // dependencies
  GLFWwindow *window{nullptr};
  VkInstance instance{VK_NULL_HANDLE};
  VkSurfaceKHR surface{VK_NULL_HANDLE};
  VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
  VkDevice device{VK_NULL_HANDLE};
  VkRenderPass renderPass{VK_NULL_HANDLE};
  VkQueue graphicsQueue{VK_NULL_HANDLE};
  VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
  VkCommandPool commandPool{VK_NULL_HANDLE};

  // components
  std::unique_ptr<FPSPanel> fps_panel;

  bool _visible{false};
};
} // namespace vkr
