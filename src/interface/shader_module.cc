#include "vkr/interface/shader_module.hpp"
#include "vkr/utils.hpp"

ShaderModule::ShaderModule(VkDevice device, const std::string &filepath)
    : device(device) {
  auto code = readFile(filepath);
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
}

ShaderModule::~ShaderModule() {
  if (shaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device, shaderModule, nullptr);
  }
}
