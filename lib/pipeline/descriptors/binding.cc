#include "vkr/pipeline/descriptors/binding.hh"
#include "vkr/logger.hh"

namespace vkr::pipeline {

DescriptorSetLayout::DescriptorSetLayout(
    const core::Device &device, const std::vector<DescriptorBinding> &bindings)
    : device_(device), bindings_(bindings) {
  VKR_PIPE_INFO("Creating descriptor set layout({} bindings)", bindings.size());

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

    VKR_PIPE_TRACE("Added binding: {}", binding.toString());
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
  layoutInfo.pBindings = vkBindings.data();

  VkResult result = vkCreateDescriptorSetLayout(device.device(), &layoutInfo,
                                                nullptr, &layout_);
  if (result != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create descriptor set layout! VkResult: {}",
                   std::to_string(result));
  }

  VKR_PIPE_INFO("Descriptor set layout created successfully.");
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
