#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace vkr {

class ShaderModule {
public:
  ShaderModule(VkDevice device, const std::string &filepath);
  ~ShaderModule();

  ShaderModule(const ShaderModule &) = delete;
  ShaderModule &operator=(const ShaderModule &) = delete;

  [[nodiscard]] VkShaderModule module() const noexcept { return _shaderModule; }

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};

  // components
  VkShaderModule _shaderModule{VK_NULL_HANDLE};
};
} // namespace vkr
