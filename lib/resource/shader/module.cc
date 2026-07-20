#include "vkr/resource/shader/module.hh"
#include "vkr/logger.hh"
#include "vkr/util/io.hh"
#include <exception>
#include <utility>

namespace vkr::resource {

ShaderModule::ShaderModule(const core::Device &device) : device_(device) {}

ShaderModule::~ShaderModule() { destroy(); }

void ShaderModule::create() {
  if (!desc_.isValid()) {
    VKR_RES_ERROR("Invalid shader module descriptor");
    return;
  }

  auto code = loadSpirv();

  if (code.empty()) {
    VKR_RES_ERROR("Shader module SPIR-V is empty");
    return;
  }

  VkShaderModuleCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = code.size() * sizeof(uint32_t);
  info.pCode = code.data();

  if (vkCreateShaderModule(device_.device(), &info, nullptr, &vk_module_) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to create shader module");
    vk_module_ = VK_NULL_HANDLE;
    return;
  }

  VKR_RES_INFO("Shader module created");
}

void ShaderModule::destroy() {
  if (vk_module_ != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device_.device(), vk_module_, nullptr);
    vk_module_ = VK_NULL_HANDLE;
  }
}

void ShaderModule::update(const ShaderModuleDesc &desc) {
  destroy();
  desc_ = desc;
  create();
}

auto ShaderModule::loadSpirv() const -> std::vector<uint32_t> {
  switch (desc_.sourceKind) {
  case ShaderModuleSourceKind::SpirvCode:
    return desc_.spirv;

  case ShaderModuleSourceKind::SpirvFile:
    try {
      return util::fread_uint32(desc_.spirvPath);
    } catch (const std::exception &e) {
      VKR_RES_ERROR("Failed to read SPIR-V file '{}': {}", desc_.spirvPath,
                    e.what());
      return {};
    }

  case ShaderModuleSourceKind::Glsl: {
    auto result = util::ShaderCompiler::compileGlsl(desc_.compile);

    if (!result) {
      VKR_RES_ERROR("Shader compilation failed '{}': {}", desc_.compile.label,
                    result.error);
      return {};
    }

    return std::move(result.spv);
  }
  }

  return {};
}

} // namespace vkr::resource
