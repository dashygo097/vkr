#include "vkr/pipeline/descriptor/descriptor_binding.hh"
#include <stdexcept>

namespace vkr {

DescriptorSetLayout::DescriptorSetLayout(
    VkDevice device, const std::vector<DescriptorBinding> &bindings)
    : device(device), _bindings(bindings) {

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

  VkResult result =
      vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_layout);
  if (result != VK_SUCCESS) {
    throw std::runtime_error(
        "Failed to create descriptor set layout. VkResult: " +
        std::to_string(result));
  }
}

DescriptorSetLayout::DescriptorSetLayout(
    const VulkanContext &ctx, const std::vector<DescriptorBinding> &bindings)
    : DescriptorSetLayout(ctx.device, bindings) {}

DescriptorSetLayout::~DescriptorSetLayout() { cleanup(); }

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout &&other) noexcept
    : device(other.device), _layout(other._layout),
      _bindings(std::move(other._bindings)) {
  other._layout = VK_NULL_HANDLE;
  other.device = VK_NULL_HANDLE;
}

DescriptorSetLayout &
DescriptorSetLayout::operator=(DescriptorSetLayout &&other) noexcept {
  if (this != &other) {
    cleanup();
    device = other.device;
    _layout = other._layout;
    _bindings = std::move(other._bindings);
    other._layout = VK_NULL_HANDLE;
    other.device = VK_NULL_HANDLE;
  }
  return *this;
}

DescriptorSetLayout DescriptorSetLayout::createDefault3D(VkDevice device) {
  std::vector<DescriptorBinding> bindings = {
      {0, DescriptorType::UniformBuffer, 1, VK_SHADER_STAGE_VERTEX_BIT}};
  return DescriptorSetLayout(device, bindings);
}

DescriptorSetLayout
DescriptorSetLayout::createDefault3D(const VulkanContext &ctx) {
  return createDefault3D(ctx.device);
}

void DescriptorSetLayout::cleanup() {
  if (device != VK_NULL_HANDLE && _layout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device, _layout, nullptr);
    _layout = VK_NULL_HANDLE;
  }
}

} // namespace vkr
