#include "vkr/pipeline/shader_module.hh"
#include "vkr/io_utils.hh"

namespace vkr {
ShaderModule::ShaderModule(const Device &device, const std::string &filepath)
    : device(device) {
  auto code = read_file(filepath);
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  if (vkCreateShaderModule(device.device(), &createInfo, nullptr,
                           &_shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
}

ShaderModule::~ShaderModule() {
  if (_shaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device.device(), _shaderModule, nullptr);
  }
}
} // namespace vkr
