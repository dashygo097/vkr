#pragma once

#include "vkr/core/instance.hh"
#include "vkr/core/surface.hh"
#include <algorithm>

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

  [[nodiscard]] auto hasExtension(const char *extension) const noexcept
      -> bool {
    return std::find(extensions.begin(), extensions.end(), extension) !=
           extensions.end();
  }

#ifdef __APPLE__
  [[nodiscard]] auto isValid() const noexcept -> bool {
    return hasExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME) &&
           hasExtension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
  }
#else
  [[nodiscard]] auto isValid() const noexcept -> bool {
    return hasExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  }
#endif

  template <typename Archive> auto serialize(Archive &ar) -> void {}
};

class Device {
public:
  explicit Device(const Instance &instance, const Surface &surface,
                  DeviceDesc &deviceDesc);
  ~Device();

  Device(const Device &) = delete;
  auto operator=(const Device &) -> Device & = delete;

  [[nodiscard]] auto desc() const noexcept -> const DeviceDesc & {
    return desc_;
  }

  void waitIdle() const;

  [[nodiscard]] auto device() const noexcept -> VkDevice {
    return vk_logical_device_;
  }
  [[nodiscard]] auto physicalDevice() const noexcept -> VkPhysicalDevice {
    return vk_physical_device_;
  }
  [[nodiscard]] auto graphicsFamily() const -> uint32_t {
    return graphics_family_;
  }
  [[nodiscard]] auto presentFamily() const -> uint32_t {
    return present_family_;
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
  DeviceDesc &desc_;
  VkPhysicalDevice vk_physical_device_{VK_NULL_HANDLE};
  VkDevice vk_logical_device_{VK_NULL_HANDLE};

  uint32_t graphics_family_{VK_QUEUE_FAMILY_IGNORED};
  uint32_t present_family_{VK_QUEUE_FAMILY_IGNORED};

  VkQueue vk_graphics_queue_{VK_NULL_HANDLE};
  VkQueue vk_present_queue_{VK_NULL_HANDLE};

  // helpers
  void pickPhysicalDevice();
  void createLogicalDevice();
  [[nodiscard]] auto isComplete() const noexcept -> bool {
    bool graphicsComplete = graphics_family_ != VK_QUEUE_FAMILY_IGNORED;
    bool presentComplete = present_family_ != VK_QUEUE_FAMILY_IGNORED;
    return graphicsComplete && presentComplete;
  }
};
} // namespace vkr::core
