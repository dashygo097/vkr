#pragma once

#include <vector>
#include <vulkan/vulkan.h>

std::vector<const char *>
getRequiredExtensions(std::vector<const char *> preExtensions);
