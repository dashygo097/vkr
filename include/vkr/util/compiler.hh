#pragma once

#include <cstdint>
#include <shaderc/shaderc.hpp>
#include <string>
#include <vector>

namespace vkr::util {

struct ShaderCompileDesc {
  shaderc_shader_kind stage{shaderc_glsl_vertex_shader};

  std::string path{};
  std::string source{};
  std::string label{"shader"};
  std::string entryPoint{"main"};

  shaderc_optimization_level optimization{
      shaderc_optimization_level_performance};
  bool generateDebugInfo{false};
  bool warningsAsErrors{false};

  uint32_t targetEnvVersion{shaderc_env_version_vulkan_1_0};

  [[nodiscard]] static auto glslFile(shaderc_shader_kind stage,
                                     const std::string &path)
      -> ShaderCompileDesc {
    ShaderCompileDesc desc{};
    desc.stage = stage;
    desc.path = path;
    desc.label = path;
    return desc;
  }

  [[nodiscard]] static auto glslSource(shaderc_shader_kind stage,
                                       const std::string &source,
                                       const std::string &label = "shader")
      -> ShaderCompileDesc {
    ShaderCompileDesc desc{};
    desc.stage = stage;
    desc.source = source;
    desc.label = label;
    return desc;
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return (!path.empty() || !source.empty()) && !entryPoint.empty();
  }
};

struct ShaderCompileResult {
  bool success{false};
  std::vector<uint32_t> spv{};
  std::string error{};

  [[nodiscard]] explicit operator bool() const noexcept { return success; }
};

class ShaderCompiler {
public:
  ShaderCompiler() = delete;

  [[nodiscard]] static auto compileGlsl(const ShaderCompileDesc &desc)
      -> ShaderCompileResult;

private:
  [[nodiscard]] static auto loadSource(const ShaderCompileDesc &desc)
      -> std::string;
};

} // namespace vkr::util
