#pragma once

#include "vkr/core/debug_messenger.hh"
#include <memory>
#include <string>
#include <vector>

namespace vkr::core {

enum class SurfaceIntegration {
  None,
  GLFW,
};

struct InstanceDesc {
  std::string name{};
  uint32_t version{VK_MAKE_VERSION(1, 0, 0)};
  SurfaceIntegration surfaceIntegration{SurfaceIntegration::GLFW};
  std::vector<std::string> requiredExtensions{};
  std::vector<std::string> optionalExtensions{};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return !name.empty() && version != 0;
  }

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

  [[nodiscard]] auto instance() const noexcept -> VkInstance {
    return vk_instance_;
  }

  [[nodiscard]] auto availableExtensions() const noexcept
      -> const std::vector<VkExtensionProperties> & {
    return available_extensions_;
  }

  [[nodiscard]] auto availableLayers() const noexcept
      -> const std::vector<VkLayerProperties> & {
    return available_layers_;
  }

  [[nodiscard]] auto enabledExtensions() const noexcept
      -> const std::vector<std::string> & {
    return enabled_extensions_;
  }

  [[nodiscard]] auto enabledLayers() const noexcept
      -> const std::vector<std::string> & {
    return enabled_layers_;
  }

  [[nodiscard]] auto hasExtension(const std::string &extension) const noexcept
      -> bool;
  [[nodiscard]] auto hasLayer(const std::string &layer) const noexcept -> bool;

private:
  // components
  InstanceDesc &desc_;
  VkInstance vk_instance_{VK_NULL_HANDLE};
  std::vector<VkExtensionProperties> available_extensions_{};
  std::vector<VkLayerProperties> available_layers_{};
  std::vector<std::string> enabled_extensions_{};
  std::vector<std::string> enabled_layers_{};

#ifndef NDEBUG
  std::unique_ptr<DebugMessenger> debug_messenger_;
#endif

  void querySupport();
  void resolveExtensions();
  void resolveLayers();
  [[nodiscard]] auto supportsExtension(const std::string &extension) const
      -> bool;
  [[nodiscard]] auto supportsLayer(const std::string &layer) const -> bool;
};
} // namespace vkr::core
