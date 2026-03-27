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
  Instance &operator=(const Instance &) = delete;

  ~Instance();

  [[nodiscard]] std::string name() const noexcept { return name_; }
  [[nodiscard]] uint32_t version() const noexcept { return version_; }
  [[nodiscard]] VkInstance instance() const noexcept { return vk_instance_; }

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
