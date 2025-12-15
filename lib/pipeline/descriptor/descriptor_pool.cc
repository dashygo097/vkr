#include "vkr/pipeline/descriptor/descriptor_pool.hh"
#include <stdexcept>

namespace vkr {

DescriptorPool::DescriptorPool(VkDevice device, uint32_t maxSets,
                               const DescriptorPoolSizes &sizes)
    : device(device), _maxSets(maxSets) {

  std::vector<VkDescriptorPoolSize> poolSizes;

  if (sizes.uniformBufferCount > 0) {
    poolSizes.push_back(
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizes.uniformBufferCount});
  }

  if (sizes.storageBufferCount > 0) {
    poolSizes.push_back(
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sizes.storageBufferCount});
  }

  if (sizes.combinedImageSamplerCount > 0) {
    poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                         sizes.combinedImageSamplerCount});
  }

  if (sizes.storageImageCount > 0) {
    poolSizes.push_back(
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, sizes.storageImageCount});
  }

  if (sizes.inputAttachmentCount > 0) {
    poolSizes.push_back(
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, sizes.inputAttachmentCount});
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = maxSets;

  VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &_pool);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool. VkResult: " +
                             std::to_string(result));
  }
}

DescriptorPool::DescriptorPool(const VulkanContext &ctx, uint32_t maxSets,
                               const DescriptorPoolSizes &sizes)
    : DescriptorPool(ctx.device, maxSets, sizes) {}

DescriptorPool::~DescriptorPool() { cleanup(); }

DescriptorPool::DescriptorPool(DescriptorPool &&other) noexcept
    : device(other.device), _pool(other._pool), _maxSets(other._maxSets),
      _allocatedSets(other._allocatedSets) {
  other._pool = VK_NULL_HANDLE;
  other.device = VK_NULL_HANDLE;
}

DescriptorPool &DescriptorPool::operator=(DescriptorPool &&other) noexcept {
  if (this != &other) {
    cleanup();
    device = other.device;
    _pool = other._pool;
    _maxSets = other._maxSets;
    _allocatedSets = other._allocatedSets;
    other._pool = VK_NULL_HANDLE;
    other.device = VK_NULL_HANDLE;
  }
  return *this;
}

bool DescriptorPool::canAllocate() const noexcept {
  return _allocatedSets < _maxSets;
}

void DescriptorPool::reset() {
  if (_pool != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
    vkResetDescriptorPool(device, _pool, 0);
    _allocatedSets = 0;
  }
}

void DescriptorPool::cleanup() {
  if (device != VK_NULL_HANDLE && _pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device, _pool, nullptr);
    _pool = VK_NULL_HANDLE;
  }
}

} // namespace vkr
