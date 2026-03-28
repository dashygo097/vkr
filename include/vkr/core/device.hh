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
  auto operator=(const Device &) -> Device & = delete;

  void waitIdle();

  [[nodiscard]] auto device() const noexcept -> VkDevice { return vk_logical_device_; }
  [[nodiscard]] auto physicalDevice() const noexcept -> VkPhysicalDevice {
    return vk_physical_device_;
  }
  [[nodiscard]] auto graphicsQueue() const noexcept -> VkQueue {
    return vk_graphics_queue_;
  }
  [[nodiscard]] auto presentQueue() const noexcept -> VkQueue {
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

  [[nodiscard]] auto isSuitable(VkPhysicalDevice physicalDevice) -> bool;
};
} // namespace vkr::core
