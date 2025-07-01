#pragma once

#include <string>
#include <vulkan/vulkan.h>

class ShaderModule {
public:
  ShaderModule(VkDevice device, const std::string &filepath);
  ~ShaderModule();

  VkShaderModule getModule() const { return shaderModule; }

private:
  // dependencies
  VkDevice device;
  // components
  VkShaderModule shaderModule{VK_NULL_HANDLE};
};
