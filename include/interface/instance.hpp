#pragma once

#include <vector>

#include "interface/debug_messenger.hpp"

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

class Instance {
public:
  Instance(VkApplicationInfo appInfo, std::vector<const char *> preExtensions,
           std::vector<const char *> validationLayers);
  ~Instance();

  VkInstance getVkInstance() const { return instance; }

private:
  // components
  VkInstance instance;
  std::unique_ptr<DebugMessenger> debugMessenger;
};
