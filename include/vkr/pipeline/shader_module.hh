#pragma once

#include "../core/device.hh"

namespace vkr::pipeline {

class ShaderModule {
public:
  explicit ShaderModule(const core::Device &device,
                        const std::string &filePath);
  explicit ShaderModule(const core::Device &device,
                        const std::vector<uint32_t> &spvCode);
  ~ShaderModule();

  ShaderModule(const ShaderModule &) = delete;
  auto operator=(const ShaderModule &) -> ShaderModule & = delete;

  [[nodiscard]] auto module() const noexcept -> VkShaderModule {
    return vk_shader_module_;
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  VkShaderModule vk_shader_module_{VK_NULL_HANDLE};
};
} // namespace vkr::pipeline
