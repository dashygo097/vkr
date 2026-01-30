#pragma once

#include "./instance.hh"
#include "./surface.hh"

namespace vkr::core {

class Device {
public:
  explicit Device(const Instance &instance, const Surface &surface,
                  const std::vector<const char *> &deviceExtensions,
                  const std::vector<const char *> &validationLayers);
  ~Device();

  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;

  void waitIdle();

  [[nodiscard]] VkDevice device() const noexcept { return vk_logical_device_; }
  [[nodiscard]] VkPhysicalDevice physicalDevice() const noexcept {
    return vk_physical_device_;
  }
  [[nodiscard]] VkQueue graphicsQueue() const noexcept {
    return vk_graphics_queue_;
  }
  [[nodiscard]] VkQueue presentQueue() const noexcept {
    return vk_present_queue_;
  }

private:
  // dependencies
  const Instance &instance_;
  const Surface &surface_;

  // components
  VkPhysicalDevice vk_physical_device_{VK_NULL_HANDLE};
  VkDevice vk_logical_device_{VK_NULL_HANDLE};
  VkQueue vk_graphics_queue_{VK_NULL_HANDLE};
  VkQueue vk_present_queue_{VK_NULL_HANDLE};

  void pickPhysicalDevice();
  void createLogicalDevice(std::vector<const char *> deviceExtensions,
                           std::vector<const char *> validationLayers);

  [[nodiscard]] bool isSuitable(VkPhysicalDevice physicalDevice);
};
} // namespace vkr::core
