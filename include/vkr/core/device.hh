#pragma once

#include "./instance.hh"
#include "./surface.hh"

namespace vkr {

class Device {
public:
  explicit Device(const Instance &instance, const Surface &surface,
                  const std::vector<const char *> &deviceExtensions,
                  const std::vector<const char *> &validationLayers);
  ~Device();

  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;

  void waitIdle();

  [[nodiscard]] VkDevice device() const noexcept { return _device; }
  [[nodiscard]] VkPhysicalDevice physicalDevice() const noexcept {
    return _physicalDevice;
  }
  [[nodiscard]] VkQueue graphicsQueue() const noexcept {
    return _graphicsQueue;
  }
  [[nodiscard]] VkQueue presentQueue() const noexcept { return _presentQueue; }

private:
  // dependencies
  const Instance &instance;
  const Surface &surface;

  // components
  VkPhysicalDevice _physicalDevice{VK_NULL_HANDLE};
  VkDevice _device{VK_NULL_HANDLE};
  VkQueue _graphicsQueue{VK_NULL_HANDLE};
  VkQueue _presentQueue{VK_NULL_HANDLE};

  void pickPhysicalDevice();
  void createLogicalDevice(std::vector<const char *> deviceExtensions,
                           std::vector<const char *> validationLayers);

  [[nodiscard]] bool isSuitable(VkPhysicalDevice pDevice);
};
} // namespace vkr
