#include "vkr/interface/shader_module.hpp"
#include "vkr/utils.hpp"

namespace vkr {
ShaderModule::ShaderModule(VkDevice device, const std::string &filepath)
    : device(device) {
  auto code = readFile(filepath);
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  if (vkCreateShaderModule(device, &createInfo, nullptr, &_shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
}

ShaderModule::~ShaderModule() {
  if (_shaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device, _shaderModule, nullptr);
  }
}
} // namespace vkr
