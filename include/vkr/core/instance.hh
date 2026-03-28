#pragma once

#include "./debug_messenger.hh"
#include <vector>

namespace vkr::core {

class Instance {
public:
  explicit Instance(std::string appName, uint32_t appVersion,
                    const std::vector<const char *> &preExtensions,
                    const std::vector<const char *> &validationLayers);

  Instance(const Instance &) = delete;
  auto operator=(const Instance &) -> Instance & = delete;

  ~Instance();

  [[nodiscard]] auto name() const noexcept -> std::string { return name_; }
  [[nodiscard]] auto version() const noexcept -> uint32_t { return version_; }
  [[nodiscard]] auto instance() const noexcept -> VkInstance { return vk_instance_; }

private:
  // components
  std::string name_{};
  uint32_t version_;
  VkInstance vk_instance_{VK_NULL_HANDLE};

#ifndef NDEBUG
  std::unique_ptr<DebugMessenger> debug_messenger_;
#endif
};
} // namespace vkr::core
