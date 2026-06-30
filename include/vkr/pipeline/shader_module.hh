#pragma once

#include "vkr/core/device.hh"
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::pipeline {

enum class ShaderSourceKind {
  SpirvFile,
  SpirvCode,
};

struct ShaderModuleDesc {
  ShaderSourceKind sourceKind{ShaderSourceKind::SpirvCode};

  std::string path{};
  std::vector<uint32_t> code{};

  [[nodiscard]] static auto spirvFile(const std::string &path)
      -> ShaderModuleDesc {
    ShaderModuleDesc desc{};
    desc.sourceKind = ShaderSourceKind::SpirvFile;
    desc.path = path;
    return desc;
  }

  [[nodiscard]] static auto spirvCode(const std::vector<uint32_t> &code)
      -> ShaderModuleDesc {
    ShaderModuleDesc desc{};
    desc.sourceKind = ShaderSourceKind::SpirvCode;
    desc.code = code;
    return desc;
  }

  [[nodiscard]] static auto spirvCode(std::vector<uint32_t> &&code)
      -> ShaderModuleDesc {
    ShaderModuleDesc desc{};
    desc.sourceKind = ShaderSourceKind::SpirvCode;
    desc.code = std::move(code);
    return desc;
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    switch (sourceKind) {
    case ShaderSourceKind::SpirvFile:
      return !path.empty();
    case ShaderSourceKind::SpirvCode:
      return !code.empty();
    }

    return false;
  }
};

class ShaderModule {
public:
  explicit ShaderModule(const core::Device &device);
  ~ShaderModule();

  ShaderModule(const ShaderModule &) = delete;
  auto operator=(const ShaderModule &) -> ShaderModule & = delete;

  void create();
  void destroy();
  void update(const ShaderModuleDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const ShaderModuleDesc & {
    return desc_;
  }

  [[nodiscard]] auto module() const noexcept -> VkShaderModule {
    return vk_shader_module_;
  }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return vk_shader_module_ != VK_NULL_HANDLE;
  }

private:
  // dependencies
  const core::Device *device_{nullptr};

  // components
  ShaderModuleDesc desc_{};
  VkShaderModule vk_shader_module_{VK_NULL_HANDLE};

  // helpers
  [[nodiscard]] auto loadSpirvCode() const -> std::vector<uint32_t>;
  void createFromCode(const std::vector<uint32_t> &code);
};

} // namespace vkr::pipeline
