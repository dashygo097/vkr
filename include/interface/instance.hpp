#pragma once

#include <string>
#include <vector>

#include "ctx.hpp"
#include "interface/debug_messenger.hpp"

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

class Instance {
public:
  Instance(const std::string appName, const std::string engineName,
           uint32_t appVersion, uint32_t engineVersion,
           const std::vector<const char *> &preExtensions,
           const std::vector<const char *> &validationLayers);
  Instance(const VulkanContext &ctx);

  ~Instance();

  VkInstance getVkInstance() const { return instance; }

private:
  // components
  VkInstance instance{VK_NULL_HANDLE};
  std::unique_ptr<DebugMessenger> debugMessenger;
};
