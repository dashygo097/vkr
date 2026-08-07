#pragma once

#include "vkr/core/device.hh"
#include "vkr/util/compiler.hh"

namespace vkr::resource {

enum class ShaderModuleSourceKind {
  SpirvCode,
  SpirvFile,
  Glsl,
  Slang,
};

struct ShaderModuleDesc {
  ShaderModuleSourceKind sourceKind{ShaderModuleSourceKind::SpirvCode};

  std::vector<uint32_t> spirv{};
  std::string spirvPath{};
  util::GlslCompileDesc glslCompile{};
  util::SlangCompileDesc slangCompile{};

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

  [[nodiscard]] static auto glsl(const util::GlslCompileDesc &compileDesc)
      -> ShaderModuleDesc {
    ShaderModuleDesc desc{};
    desc.sourceKind = ShaderModuleSourceKind::Glsl;
    desc.glslCompile = compileDesc;
    return desc;
  }

  [[nodiscard]] static auto glsl(util::GlslCompileDesc &&compileDesc)
      -> ShaderModuleDesc {
    ShaderModuleDesc desc{};
    desc.sourceKind = ShaderModuleSourceKind::Glsl;
    desc.glslCompile = std::move(compileDesc);
    return desc;
  }

  [[nodiscard]] static auto slang(const util::SlangCompileDesc &compile)
      -> ShaderModuleDesc {
    ShaderModuleDesc desc{};
    desc.sourceKind = ShaderModuleSourceKind::Slang;
    desc.slangCompile = compile;
    return desc;
  }

  [[nodiscard]] static auto slang(util::SlangCompileDesc &&compile)
      -> ShaderModuleDesc {
    ShaderModuleDesc desc{};
    desc.sourceKind = ShaderModuleSourceKind::Slang;
    desc.slangCompile = std::move(compile);
    return desc;
  }

  [[nodiscard]] static auto vertexGlslFile(const std::string &path)
      -> ShaderModuleDesc {
    return glsl(
        util::GlslCompileDesc::glslFile(shaderc_glsl_vertex_shader, path));
  }

  [[nodiscard]] static auto fragmentGlslFile(const std::string &path)
      -> ShaderModuleDesc {
    return glsl(
        util::GlslCompileDesc::glslFile(shaderc_glsl_fragment_shader, path));
  }

  [[nodiscard]] static auto computeGlslFile(const std::string &path)
      -> ShaderModuleDesc {
    return glsl(
        util::GlslCompileDesc::glslFile(shaderc_glsl_compute_shader, path));
  }

  [[nodiscard]] static auto computeSlangFile(const std::string &path)
      -> ShaderModuleDesc {
    return slang(util::SlangCompileDesc::computeFile(path));
  }

  [[nodiscard]] static auto
  vertexGlslSource(const std::string &source,
                   const std::string &label = "vertex") -> ShaderModuleDesc {
    return glsl(util::GlslCompileDesc::glslSource(shaderc_glsl_vertex_shader,
                                                  source, label));
  }

  [[nodiscard]] static auto
  fragmentGlslSource(const std::string &source,
                     const std::string &label = "fragment")
      -> ShaderModuleDesc {
    return glsl(util::GlslCompileDesc::glslSource(shaderc_glsl_fragment_shader,
                                                  source, label));
  }

  [[nodiscard]] static auto
  computeGlslSource(const std::string &source,
                    const std::string &label = "compute") -> ShaderModuleDesc {
    return glsl(util::GlslCompileDesc::glslSource(shaderc_glsl_compute_shader,
                                                  source, label));
  }

  [[nodiscard]] static auto
  computeSlangSource(const std::string &source,
                     const std::string &label = "compute") -> ShaderModuleDesc {
    return slang(util::SlangCompileDesc::computeSource(source, label));
  }

  void setEntryPoint(const std::string &entryPoint) {
    if (sourceKind == ShaderModuleSourceKind::Glsl) {
      glslCompile.entryPoint = entryPoint;
    } else if (sourceKind == ShaderModuleSourceKind::Slang) {
      slangCompile.entryPoint = entryPoint;
    }
  }

  [[nodiscard]] auto label() const noexcept -> const std::string & {
    if (sourceKind == ShaderModuleSourceKind::Glsl) {
      return glslCompile.label;
    }

    if (sourceKind == ShaderModuleSourceKind::Slang) {
      return slangCompile.label;
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
      return glslCompile.isValid();
    case ShaderModuleSourceKind::Slang:
      return slangCompile.isValid();
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
