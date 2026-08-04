#include "vkr/util/compiler.hh"
#include "vkr/logger.hh"
#include "vkr/util/io.hh"

namespace vkr::util {

auto ShaderCompiler::compileGlsl(const ShaderCompileDesc &desc)
    -> ShaderCompileResult {
  ShaderCompileResult result{};

  if (!desc.isValid()) {
    result.error = "invalid shader compile descriptor";
    return result;
  }

  const std::string source = loadSource(desc);
  if (source.empty()) {
    result.error = "shader source is empty: " + desc.label;
    return result;
  }

  shaderc::Compiler compiler;
  shaderc::CompileOptions options;
  options.SetSourceLanguage(shaderc_source_language_glsl);
  options.SetOptimizationLevel(desc.optimization);
  options.SetTargetEnvironment(shaderc_target_env_vulkan,
                               desc.targetEnvVersion);

  if (desc.generateDebugInfo) {
    options.SetGenerateDebugInfo();
  }

  if (desc.warningsAsErrors) {
    options.SetWarningsAsErrors();
  }

  VKR_UTIL_INFO("Compiling shader: {}", desc.label);

  auto compiled = compiler.CompileGlslToSpv(
      source, desc.stage, desc.label.c_str(), desc.entryPoint.c_str(), options);

  if (compiled.GetCompilationStatus() != shaderc_compilation_status_success) {
    result.success = false;
    result.error = compiled.GetErrorMessage();
    VKR_UTIL_WARN("Shader compilation failed: {}", result.error);
    return result;
  }

  result.success = true;
  result.spv = {compiled.cbegin(), compiled.cend()};
  return result;
}

auto ShaderCompiler::loadSource(const ShaderCompileDesc &desc) -> std::string {
  if (!desc.source.empty()) {
    return desc.source;
  }

  return fread_string(desc.path);
}

} // namespace vkr::util
