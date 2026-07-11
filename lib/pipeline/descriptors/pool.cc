#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/logger.hh"

namespace vkr::pipeline {

DescriptorPool::DescriptorPool(const core::Device &device) : device_(device) {}

DescriptorPool::~DescriptorPool() { destroy(); }

void DescriptorPool::create() {
  destroy();

  if (desc_.maxSets == 0) {
    VKR_PIPE_TRACE("Descriptor pool creation skipped because maxSets is 0");
    return;
  }

  if (desc_.poolSizes.empty()) {
    VKR_PIPE_ERROR(
        "Cannot create descriptor pool with maxSets={} and no pool sizes",
        desc_.maxSets);
  }

  for (const auto &poolSize : desc_.poolSizes) {
    if (poolSize.descriptorCount == 0) {
      VKR_PIPE_ERROR("Descriptor pool size for type {} has zero count",
                     static_cast<int>(poolSize.type));
    }
  }

  VKR_PIPE_INFO("Creating descriptor pool(maxSets={}, poolSizes={})",
                desc_.maxSets, desc_.poolSizes.size());

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = desc_.flags;
  poolInfo.maxSets = desc_.maxSets;
  poolInfo.poolSizeCount = static_cast<uint32_t>(desc_.poolSizes.size());
  poolInfo.pPoolSizes = desc_.poolSizes.data();

  const VkResult result =
      vkCreateDescriptorPool(device_.device(), &poolInfo, nullptr, &pool_);

  if (result != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create descriptor pool. VkResult: {}",
                   static_cast<int>(result));
  }

  desc_.allocatedSets = 0;

  VKR_PIPE_INFO("Descriptor pool created successfully.");
}

void DescriptorPool::destroy() {
  if (pool_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device_.device(), pool_, nullptr);
    pool_ = VK_NULL_HANDLE;
  }

  desc_.allocatedSets = 0;
}

void DescriptorPool::update(const DescriptorPoolDesc &desc) {
  desc_ = desc;
  desc_.allocatedSets = 0;
  create();
}

} // namespace vkr::pipeline
