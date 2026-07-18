#pragma once

#include "vkr/core/instance.hh"
#include "vkr/core/surface.hh"
#include <algorithm>
#include <vector>

#ifdef __APPLE__
#include <vulkan/vulkan_beta.h>
#endif

namespace vkr::core {

struct QueueFamilySupport {
  std::vector<VkQueueFamilyProperties> families{};

  uint32_t graphicsFamily{VK_QUEUE_FAMILY_IGNORED};
  uint32_t presentFamily{VK_QUEUE_FAMILY_IGNORED};
  uint32_t computeFamily{VK_QUEUE_FAMILY_IGNORED};
  uint32_t transferFamily{VK_QUEUE_FAMILY_IGNORED};

  [[nodiscard]] auto supportsGraphics() const noexcept -> bool {
    return graphicsFamily != VK_QUEUE_FAMILY_IGNORED;
  }

  [[nodiscard]] auto supportsPresent() const noexcept -> bool {
    return presentFamily != VK_QUEUE_FAMILY_IGNORED;
  }

  [[nodiscard]] auto supportsCompute() const noexcept -> bool {
    return computeFamily != VK_QUEUE_FAMILY_IGNORED;
  }

  [[nodiscard]] auto supportsTransfer() const noexcept -> bool {
    return transferFamily != VK_QUEUE_FAMILY_IGNORED;
  }

  [[nodiscard]] auto supportsGraphicsPresentation() const noexcept -> bool {
    return supportsGraphics() && supportsPresent();
  }
};

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
    return queue_family_support_.graphicsFamily;
  }
  [[nodiscard]] auto presentFamily() const -> uint32_t {
    return queue_family_support_.presentFamily;
  }
  [[nodiscard]] auto computeFamily() const -> uint32_t {
    return queue_family_support_.computeFamily;
  }
  [[nodiscard]] auto transferFamily() const -> uint32_t {
    return queue_family_support_.transferFamily;
  }
  [[nodiscard]] auto queueFamilySupport() const noexcept
      -> const QueueFamilySupport & {
    return queue_family_support_;
  }
  [[nodiscard]] auto supportsGraphics() const noexcept -> bool {
    return queue_family_support_.supportsGraphics();
  }
  [[nodiscard]] auto supportsPresent() const noexcept -> bool {
    return queue_family_support_.supportsPresent();
  }
  [[nodiscard]] auto supportsCompute() const noexcept -> bool {
    return queue_family_support_.supportsCompute();
  }
  [[nodiscard]] auto supportsTransfer() const noexcept -> bool {
    return queue_family_support_.supportsTransfer();
  }
  [[nodiscard]] auto graphicsQueue() const noexcept -> VkQueue {
    return vk_graphics_queue_;
  }
  [[nodiscard]] auto presentQueue() const noexcept -> VkQueue {
    return vk_present_queue_;
  }
  [[nodiscard]] auto computeQueue() const noexcept -> VkQueue {
    return vk_compute_queue_;
  }
  [[nodiscard]] auto transferQueue() const noexcept -> VkQueue {
    return vk_transfer_queue_;
  }

private:
  // dependencies
  const Instance &instance_;
  const Surface &surface_;

  // components
  DeviceDesc &desc_;
  VkPhysicalDevice vk_physical_device_{VK_NULL_HANDLE};
  VkDevice vk_logical_device_{VK_NULL_HANDLE};

  QueueFamilySupport queue_family_support_{};

  VkQueue vk_graphics_queue_{VK_NULL_HANDLE};
  VkQueue vk_present_queue_{VK_NULL_HANDLE};
  VkQueue vk_compute_queue_{VK_NULL_HANDLE};
  VkQueue vk_transfer_queue_{VK_NULL_HANDLE};

  // helpers
  void pickPhysicalDevice();
  void createLogicalDevice();
  [[nodiscard]] auto isComplete() const noexcept -> bool {
    return queue_family_support_.supportsGraphicsPresentation();
  }
  [[nodiscard]] auto queryQueueFamilySupport(VkPhysicalDevice device) const
      -> QueueFamilySupport;
};
} // namespace vkr::core
