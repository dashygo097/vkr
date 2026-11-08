#pragma once

#include "vkr/core/instance.hh"
#include "vkr/core/surface.hh"
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace vkr::core {

struct DeviceDesc {
  std::vector<std::string> requiredExtensions{};
  std::vector<std::string> optionalExtensions{};

  [[nodiscard]] auto isValid() const noexcept -> bool { return true; }

  template <typename Archive> auto serialize(Archive &ar) -> void {}
};

class Device {
public:
  explicit Device(const Instance &instance, DeviceDesc &deviceDesc);
  explicit Device(const Instance &instance, const Surface &surface,
                  DeviceDesc &deviceDesc);
  ~Device();

  Device(const Device &) = delete;
  auto operator=(const Device &) -> Device & = delete;

  void waitIdle() const;

  [[nodiscard]] auto device() const noexcept -> VkDevice {
    return vk_logical_device_;
  }
  [[nodiscard]] auto physicalDevice() const noexcept -> VkPhysicalDevice {
    return vk_physical_device_;
  }
  [[nodiscard]] auto apiVersion() const noexcept -> uint32_t {
    return api_version_;
  }
  [[nodiscard]] auto cmdBeginRendering() const noexcept
      -> PFN_vkCmdBeginRendering {
    return cmd_begin_rendering_;
  }
  [[nodiscard]] auto cmdEndRendering() const noexcept -> PFN_vkCmdEndRendering {
    return cmd_end_rendering_;
  }
  [[nodiscard]] auto physicalDevicefeatures() const noexcept
      -> const VkPhysicalDeviceFeatures & {
    return physical_device_features_;
  }
  [[nodiscard]] auto physicalDeviceproperties() const noexcept
      -> const VkPhysicalDeviceProperties & {
    return physical_device_properties_;
  }
  [[nodiscard]] auto formatProperties(VkFormat format) const
      -> VkFormatProperties {
    VkFormatProperties properties{};
    vkGetPhysicalDeviceFormatProperties(vk_physical_device_, format,
                                        &properties);
    return properties;
  }
  [[nodiscard]] auto availableExtensions() const noexcept
      -> const std::vector<VkExtensionProperties> & {
    return available_extensions_;
  }
  [[nodiscard]] auto enabledExtensions() const noexcept
      -> const std::vector<std::string> & {
    return enabled_extensions_;
  }
  [[nodiscard]] auto queueFamilies() const noexcept
      -> const std::vector<VkQueueFamilyProperties> & {
    return queue_families_;
  }
  [[nodiscard]] auto graphicsFamily() const -> uint32_t {
    return graphics_family_;
  }
  [[nodiscard]] auto presentFamily() const -> uint32_t {
    return present_family_;
  }
  [[nodiscard]] auto computeFamily() const -> uint32_t {
    return compute_family_;
  }
  [[nodiscard]] auto transferFamily() const -> uint32_t {
    return transfer_family_;
  }
  [[nodiscard]] auto supportsGraphics() const noexcept -> bool {
    return graphics_family_ != VK_QUEUE_FAMILY_IGNORED;
  }
  [[nodiscard]] auto supportsPresent() const noexcept -> bool {
    return present_family_ != VK_QUEUE_FAMILY_IGNORED;
  }
  [[nodiscard]] auto supportsCompute() const noexcept -> bool {
    return compute_family_ != VK_QUEUE_FAMILY_IGNORED;
  }
  [[nodiscard]] auto supportsTransfer() const noexcept -> bool {
    return transfer_family_ != VK_QUEUE_FAMILY_IGNORED;
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
  [[nodiscard]] auto hasExtension(const std::string &extension) const noexcept
      -> bool;

private:
  // dependencies
  const Instance &instance_;
  std::optional<std::reference_wrapper<const Surface>> surface_{};

  // components
  DeviceDesc &desc_;
  VkPhysicalDevice vk_physical_device_{VK_NULL_HANDLE};
  VkDevice vk_logical_device_{VK_NULL_HANDLE};
  VkPhysicalDeviceProperties physical_device_properties_{};
  VkPhysicalDeviceFeatures physical_device_features_{};
  uint32_t api_version_{0};
  PFN_vkCmdBeginRendering cmd_begin_rendering_{nullptr};
  PFN_vkCmdEndRendering cmd_end_rendering_{nullptr};

  std::vector<VkExtensionProperties> available_extensions_{};
  std::vector<std::string> enabled_extensions_{};
  std::vector<VkQueueFamilyProperties> queue_families_{};
  uint32_t graphics_family_{VK_QUEUE_FAMILY_IGNORED};
  uint32_t present_family_{VK_QUEUE_FAMILY_IGNORED};
  uint32_t compute_family_{VK_QUEUE_FAMILY_IGNORED};
  uint32_t transfer_family_{VK_QUEUE_FAMILY_IGNORED};

  VkQueue vk_graphics_queue_{VK_NULL_HANDLE};
  VkQueue vk_present_queue_{VK_NULL_HANDLE};
  VkQueue vk_compute_queue_{VK_NULL_HANDLE};
  VkQueue vk_transfer_queue_{VK_NULL_HANDLE};

  // helpers
  void pickPhysicalDevice();
  void createLogicalDevice();
  void queryDeviceSupport(VkPhysicalDevice device);
  [[nodiscard]] auto resolveExtensions(uint32_t apiVersion) -> bool;
  void resolveQueueFamilies(VkPhysicalDevice device);
};
} // namespace vkr::core
