#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/logger.hh"

namespace vkr::pipeline {

DescriptorSetLayout::DescriptorSetLayout(const core::Device &device)
    : device_(device) {}

DescriptorSetLayout::~DescriptorSetLayout() { destroy(); }

void DescriptorSetLayout::create() {
  destroy();

  VKR_PIPE_INFO("Creating descriptor set layout({} bindings)",
                desc_.bindings.size());

  std::vector<VkDescriptorSetLayoutBinding> vkBindings{};
  vkBindings.reserve(desc_.bindings.size());

  for (const auto &binding : desc_.bindings) {
    vkBindings.push_back(binding.layout);
    VKR_PIPE_TRACE("Added binding: {}", binding.toString());
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
  layoutInfo.pBindings = vkBindings.empty() ? nullptr : vkBindings.data();

  if (vkCreateDescriptorSetLayout(device_.device(), &layoutInfo, nullptr,
                                  &layout_) != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create descriptor set layout.");
  }

  VKR_PIPE_INFO("Descriptor set layout created successfully.");
}

void DescriptorSetLayout::destroy() {
  if (layout_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device_.device(), layout_, nullptr);
    layout_ = VK_NULL_HANDLE;
  }
}

void DescriptorSetLayout::update(const DescriptorSetLayoutDesc &desc) {
  desc_ = desc;
  create();
}

} // namespace vkr::pipeline
