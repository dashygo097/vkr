#pragma once

#include "vkr/core/instance.hh"
#include "vkr/core/surface.hh"

#ifdef __APPLE__
#include <vulkan/vulkan_beta.h>
#endif

namespace vkr::core {

struct DeviceDesc {
  std::vector<const char *> extensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
      VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
#endif
  };

  [[nodiscard]] auto isValid() const noexcept -> bool { return true; }

  template <typename Archive> auto serialize(Archive &ar) -> void {}
};

class Device {
public:
  explicit Device(const Instance &instance, const Surface &surface,
                  DeviceDesc &deviceDesc);
  ~Device();

  Device(const Device &) = delete;
  auto operator=(const Device &) -> Device & = delete;

  [[nodiscard]] auto desc() const noexcept -> DeviceDesc { return desc_; }

  void waitIdle();

  [[nodiscard]] auto device() const noexcept -> VkDevice {
    return vk_logical_device_;
  }
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
  DeviceDesc &desc_;

  void pickPhysicalDevice();
  void createLogicalDevice();

  [[nodiscard]] auto isSuitable(VkPhysicalDevice physicalDevice) -> bool;
};
} // namespace vkr::core
