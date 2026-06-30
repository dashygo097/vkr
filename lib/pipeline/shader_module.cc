#include "vkr/pipeline/shader_module.hh"
#include "vkr/logger.hh"
#include "vkr/util/io.hh"
#include <cstddef>
#include <cstdint>
#include <utility>

namespace vkr::pipeline {

ShaderModule::ShaderModule(const core::Device &device) : device_(&device) {}

ShaderModule::~ShaderModule() { destroy(); }

ShaderModule::ShaderModule(ShaderModule &&other) noexcept
    : device_(other.device_), desc_(std::move(other.desc_)),
      vk_shader_module_(other.vk_shader_module_) {
  other.device_ = nullptr;
  other.vk_shader_module_ = VK_NULL_HANDLE;
}

auto ShaderModule::operator=(ShaderModule &&other) noexcept -> ShaderModule & {
  if (this == &other) {
    return *this;
  }

  destroy();

  device_ = other.device_;
  desc_ = std::move(other.desc_);
  vk_shader_module_ = other.vk_shader_module_;

  other.device_ = nullptr;
  other.vk_shader_module_ = VK_NULL_HANDLE;

  return *this;
}

void ShaderModule::create() {
  destroy();

  if (device_ == nullptr) {
    VKR_PIPE_ERROR("Cannot create shader module without device");
  }

  if (!desc_.isValid()) {
    VKR_PIPE_ERROR("Invalid ShaderModuleDesc");
  }

  auto code = loadSpirvCode();
  createFromCode(code);
}

void ShaderModule::destroy() {
  if (device_ != nullptr && vk_shader_module_ != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device_->device(), vk_shader_module_, nullptr);
    vk_shader_module_ = VK_NULL_HANDLE;
  }
}

auto ShaderModule::loadSpirvCode() const -> std::vector<uint32_t> {
  switch (desc_.sourceKind) {
  case ShaderSourceKind::SpirvFile:
    VKR_PIPE_INFO("Loading SPIR-V shader: {}", desc_.path);
    return util::fread_uint32(desc_.path);

  case ShaderSourceKind::SpirvCode:
    return desc_.code;
  }

  VKR_PIPE_ERROR("Unsupported shader source kind");
}

void ShaderModule::createFromCode(const std::vector<uint32_t> &code) {
  if (code.empty()) {
    VKR_PIPE_ERROR("Cannot create shader module from empty SPIR-V code");
  }

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size() * sizeof(uint32_t);
  createInfo.pCode = code.data();

  if (vkCreateShaderModule(device_->device(), &createInfo, nullptr,
                           &vk_shader_module_) != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create shader module");
  }

  VKR_PIPE_INFO("Shader module created successfully");
}

void ShaderModule::update(const ShaderModuleDesc &desc) {
  destroy();
  desc_ = desc;
  create();
}

} // namespace vkr::pipeline
