#include "vkr/pipeline/shader_module.hh"
#include "vkr/io_utils.hh"

namespace vkr::pipeline {
ShaderModule::ShaderModule(const core::Device &device,
                           const std::string &filePath)
    : device_(device) {
  auto code = read_file(filePath);
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  if (vkCreateShaderModule(device.device(), &createInfo, nullptr,
                           &vk_shader_module_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
}

ShaderModule::~ShaderModule() {
  if (vk_shader_module_ != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device_.device(), vk_shader_module_, nullptr);
  }
}

} // namespace vkr::pipeline
