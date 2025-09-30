#pragma once

#include "../ctx.hh"
#include "./debug_messenger.hh"

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

namespace vkr {

class Instance {
public:
  explicit Instance(const std::string appName, const std::string engineName,
                    uint32_t appVersion, uint32_t engineVersion,
                    const std::vector<const char *> &preExtensions,
                    const std::vector<const char *> &validationLayers);
  explicit Instance(const VulkanContext &ctx);

  Instance(const Instance &) = delete;
  Instance &operator=(const Instance &) = delete;

  ~Instance();

  [[nodiscard]] VkInstance instance() const noexcept { return _instance; }

private:
  // components
  VkInstance _instance{VK_NULL_HANDLE};
  std::unique_ptr<DebugMessenger> _debugMessenger;
};
} // namespace vkr
