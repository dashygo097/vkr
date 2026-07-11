#pragma once

#include "vkr/core/device.hh"
#include "vkr/util/compiler.hh"

namespace vkr::resource {

enum class ShaderModuleSourceKind {
  SpirvCode,
  SpirvFile,
  Glsl,
};

struct ShaderModuleDesc {
  ShaderModuleSourceKind sourceKind{ShaderModuleSourceKind::SpirvCode};

  std::vector<uint32_t> spirv{};
  std::string spirvPath{};
  util::ShaderCompileDesc compile{};

  [[nodiscard]] static auto spirvCode(std::vector<uint32_t> spirv)
      -> ShaderModuleDesc {
    ShaderModuleDesc desc{};
    desc.sourceKind = ShaderModuleSourceKind::SpirvCode;
    desc.spirv = std::move(spirv);
    return desc;
  }

  [[nodiscard]] static auto spirvFile(const std::string &path)
      -> ShaderModuleDesc {
    ShaderModuleDesc desc{};
    desc.sourceKind = ShaderModuleSourceKind::SpirvFile;
    desc.spirvPath = path;
    return desc;
  }

  [[nodiscard]] static auto glsl(const util::ShaderCompileDesc &compile)
      -> ShaderModuleDesc {
    ShaderModuleDesc desc{};
    desc.sourceKind = ShaderModuleSourceKind::Glsl;
    desc.compile = compile;
    return desc;
  }

  [[nodiscard]] static auto glsl(util::ShaderCompileDesc &&compile)
      -> ShaderModuleDesc {
    ShaderModuleDesc desc{};
    desc.sourceKind = ShaderModuleSourceKind::Glsl;
    desc.compile = std::move(compile);
    return desc;
  }

  [[nodiscard]] static auto vertexGlslFile(const std::string &path)
      -> ShaderModuleDesc {
    return glsl(
        util::ShaderCompileDesc::glslFile(shaderc_glsl_vertex_shader, path));
  }

  [[nodiscard]] static auto fragmentGlslFile(const std::string &path)
      -> ShaderModuleDesc {
    return glsl(
        util::ShaderCompileDesc::glslFile(shaderc_glsl_fragment_shader, path));
  }

  [[nodiscard]] static auto
  vertexGlslSource(const std::string &source,
                   const std::string &label = "vertex") -> ShaderModuleDesc {
    return glsl(util::ShaderCompileDesc::glslSource(shaderc_glsl_vertex_shader,
                                                    source, label));
  }

  [[nodiscard]] static auto
  fragmentGlslSource(const std::string &source,
                     const std::string &label = "fragment")
      -> ShaderModuleDesc {
    return glsl(util::ShaderCompileDesc::glslSource(
        shaderc_glsl_fragment_shader, source, label));
  }

  void setEntryPoint(const std::string &entryPoint) {
    if (sourceKind == ShaderModuleSourceKind::Glsl) {
      compile.entryPoint = entryPoint;
    }
  }

  [[nodiscard]] auto label() const noexcept -> const std::string & {
    if (sourceKind == ShaderModuleSourceKind::Glsl) {
      return compile.label;
    }

    return spirvPath;
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    switch (sourceKind) {
    case ShaderModuleSourceKind::SpirvCode:
      return !spirv.empty();
    case ShaderModuleSourceKind::SpirvFile:
      return !spirvPath.empty();
    case ShaderModuleSourceKind::Glsl:
      return compile.isValid();
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
    return vk_module_;
  }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return vk_module_ != VK_NULL_HANDLE;
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  ShaderModuleDesc desc_{};
  VkShaderModule vk_module_{VK_NULL_HANDLE};

  // helpers
  [[nodiscard]] auto loadSpirv() const -> std::vector<uint32_t>;
};

} // namespace vkr::resource
