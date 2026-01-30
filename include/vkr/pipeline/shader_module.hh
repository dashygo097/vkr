#pragma once

#include "../core/device.hh"

namespace vkr::pipeline {

class ShaderModule {
public:
  explicit ShaderModule(const core::Device &device,
                        const std::string &filePath);
  ~ShaderModule();

  ShaderModule(const ShaderModule &) = delete;
  ShaderModule &operator=(const ShaderModule &) = delete;

  [[nodiscard]] VkShaderModule module() const noexcept {
    return vk_shader_module_;
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  VkShaderModule vk_shader_module_{VK_NULL_HANDLE};
};
} // namespace vkr::pipeline
