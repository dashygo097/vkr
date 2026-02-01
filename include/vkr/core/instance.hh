#pragma once

#include "./debug_messenger.hh"
#include <vector>

namespace vkr::core {

class Instance {
public:
  explicit Instance(const std::string appName, uint32_t appVersion,
                    const std::vector<const char *> &preExtensions,
                    const std::vector<const char *> &validationLayers);

  Instance(const Instance &) = delete;
  Instance &operator=(const Instance &) = delete;

  ~Instance();

  [[nodiscard]] VkInstance instance() const noexcept { return vk_instance_; }

private:
  // components
  VkInstance vk_instance_{VK_NULL_HANDLE};

#ifndef NDEBUG
  std::unique_ptr<DebugMessenger> debug_messenger_;
#endif
};
} // namespace vkr::core
