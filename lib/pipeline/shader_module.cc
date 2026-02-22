#include "vkr/pipeline/shader_module.hh"
#include "vkr/io_utils.hh"
#include "vkr/logger.hh"

namespace vkr::pipeline {
ShaderModule::ShaderModule(const core::Device &device,
                           const std::string &filePath)
    : device_(device) {
  VKR_PIPE_INFO("Creating shader module from file: {}...", filePath);

  auto code = fread_char(filePath);
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  if (vkCreateShaderModule(device.device(), &createInfo, nullptr,
                           &vk_shader_module_) != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create shader module from file: {}!", filePath);
  }

  VKR_PIPE_INFO("Shader module created successfully from file: {}.", filePath);
}

ShaderModule::ShaderModule(const core::Device &device,
                           const std::vector<uint32_t> &spvCode)
    : device_(device) {
  VKR_PIPE_INFO("Creating shader module from SPIR-V code...");

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = spvCode.size() * sizeof(uint32_t);
  createInfo.pCode = spvCode.data();

  if (vkCreateShaderModule(device.device(), &createInfo, nullptr,
                           &vk_shader_module_) != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create shader module from SPIR-V code!");
  }

  VKR_PIPE_INFO("Shader module created successfully from SPIR-V code.");
}

ShaderModule::~ShaderModule() {
  if (vk_shader_module_ != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device_.device(), vk_shader_module_, nullptr);
  }
}

} // namespace vkr::pipeline
