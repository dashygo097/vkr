#pragma once

namespace vkr {

class ShaderModule {
public:
  ShaderModule(VkDevice device, const std::string &filepath);
  ~ShaderModule();

  ShaderModule(const ShaderModule &) = delete;
  ShaderModule &operator=(const ShaderModule &) = delete;

  VkShaderModule getModule() const { return shaderModule; }

private:
  // dependencies
  VkDevice device;
  // components
  VkShaderModule shaderModule{VK_NULL_HANDLE};
};
} // namespace vkr
