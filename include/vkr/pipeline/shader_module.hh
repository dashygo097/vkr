#pragma once

#include "../core/device.hh"

namespace vkr {

class ShaderModule {
public:
  explicit ShaderModule(const Device &device, const std::string &filePath);
  ~ShaderModule();

  ShaderModule(const ShaderModule &) = delete;
  ShaderModule &operator=(const ShaderModule &) = delete;

  [[nodiscard]] VkShaderModule module() const noexcept { return _shaderModule; }

private:
  // dependencies
  const Device &device;

  // components
  VkShaderModule _shaderModule{VK_NULL_HANDLE};
};
} // namespace vkr
