#pragma once

#include <vector>
#include <vulkan/vulkan.h>

VkShaderModule createShaderModule(VkDevice device,
                                  const std::vector<char> &code);
