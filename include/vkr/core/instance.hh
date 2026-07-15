#pragma once

#include "vkr/core/debug_messenger.hh"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace vkr::core {

struct InstanceDesc {
  std::string name{};
  uint32_t version{VK_MAKE_VERSION(1, 0, 0)};
  std::vector<const char *> extensions{
#ifdef __APPLE__
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
  };
  std::vector<const char *> validationLayers{
#ifdef NDEBUG
#else
      "VK_LAYER_KHRONOS_validation",
#endif
  };

  [[nodiscard]] auto hasExtension(const char *extension) const noexcept
      -> bool {
    return std::find(extensions.begin(), extensions.end(), extension) !=
           extensions.end();
  }

#ifdef __APPLE__
  [[nodiscard]] auto isValid() const noexcept -> bool {
    return !name.empty() && version != 0 &&
           hasExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) &&
           hasExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  }
#else
  [[nodiscard]] auto isValid() const noexcept -> bool {
    return !name.empty() && version != 0 &&
           hasExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  }
#endif

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("name", name);
    ar("version", version);
  }
};

class Instance {
public:
  explicit Instance(InstanceDesc &desc);
  ~Instance();

  Instance(const Instance &) = delete;
  auto operator=(const Instance &) -> Instance & = delete;

  [[nodiscard]] auto desc() const noexcept -> const InstanceDesc & {
    return desc_;
  }

  [[nodiscard]] auto instance() const noexcept -> VkInstance {
    return vk_instance_;
  }

private:
  // components
  InstanceDesc &desc_;
  VkInstance vk_instance_{VK_NULL_HANDLE};

#ifndef NDEBUG
  std::unique_ptr<DebugMessenger> debug_messenger_;
#endif
};
} // namespace vkr::core
