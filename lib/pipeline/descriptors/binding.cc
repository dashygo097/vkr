#include "vkr/pipeline/descriptors/binding.hh"
#include <stdexcept>

namespace vkr::pipeline {

DescriptorSetLayout::DescriptorSetLayout(
    const core::Device &device, const std::vector<DescriptorBinding> &bindings)
    : device_(device), bindings_(bindings) {

  std::vector<VkDescriptorSetLayoutBinding> vkBindings;
  vkBindings.reserve(bindings.size());

  for (const auto &binding : bindings) {
    VkDescriptorSetLayoutBinding vkBinding{};
    vkBinding.binding = binding.binding;
    vkBinding.descriptorType = binding.toVkType();
    vkBinding.descriptorCount = binding.count;
    vkBinding.stageFlags = binding.stageFlags;
    vkBinding.pImmutableSamplers = nullptr;
    vkBindings.push_back(vkBinding);
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
  layoutInfo.pBindings = vkBindings.data();

  VkResult result = vkCreateDescriptorSetLayout(device.device(), &layoutInfo,
                                                nullptr, &layout_);
  if (result != VK_SUCCESS) {
    throw std::runtime_error(
        "Failed to create descriptor set layout. VkResult: " +
        std::to_string(result));
  }
}

DescriptorSetLayout::~DescriptorSetLayout() { cleanup(); }

DescriptorSetLayout
DescriptorSetLayout::createDefault3D(const core::Device &device) {
  std::vector<DescriptorBinding> bindings = {
      {0, DescriptorType::UniformBuffer, 1, VK_SHADER_STAGE_VERTEX_BIT}};
  return DescriptorSetLayout(device, bindings);
}

void DescriptorSetLayout::cleanup() {
  if (layout_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device_.device(), layout_, nullptr);
    layout_ = VK_NULL_HANDLE;
  }
}

} // namespace vkr::pipeline
