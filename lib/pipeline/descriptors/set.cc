#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/logger.hh"

namespace vkr::pipeline {

DescriptorSets::DescriptorSets(const core::Device &device) : device_(device) {}

DescriptorSets::~DescriptorSets() { destroy(); }

void DescriptorSets::create() {
  destroy();

  if (desc_.setCount == 0) {
    VKR_PIPE_TRACE("Descriptor set allocation skipped because setCount is 0");
    return;
  }

  if (!desc_.canAllocate()) {
    VKR_PIPE_ERROR("Cannot allocate descriptor sets with a null pool/layout or "
                   "setCount={}",
                   desc_.setCount);
  }

  allocateSets();
  updateDescriptors();
}

void DescriptorSets::destroy() {
  if (!sets_.empty() && allocated_pool_ != VK_NULL_HANDLE) {
    vkFreeDescriptorSets(device_.device(), allocated_pool_,
                         static_cast<uint32_t>(sets_.size()), sets_.data());
  }

  sets_.clear();
  allocated_pool_ = VK_NULL_HANDLE;
}

void DescriptorSets::update(const DescriptorSetsDesc &desc) {
  desc_ = desc;
  create();
}

void DescriptorSets::allocateSets() {
  std::vector<VkDescriptorSetLayout> layouts(desc_.setCount, desc_.layout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = desc_.pool;
  allocInfo.descriptorSetCount = desc_.setCount;
  allocInfo.pSetLayouts = layouts.data();

  sets_.resize(desc_.setCount);
  VkResult result =
      vkAllocateDescriptorSets(device_.device(), &allocInfo, sets_.data());
  if (result != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to allocate descriptor sets. VkResult: {}",
                   std::to_string(result));
  }

  allocated_pool_ = desc_.pool;

  VKR_PIPE_INFO("Allocated {} descriptor sets", sets_.size());
}

void DescriptorSets::updateDescriptors() {
  if (desc_.writes.empty()) {
    return;
  }

  std::vector<VkWriteDescriptorSet> writes{};

  for (const auto &setWrite : desc_.writes) {
    if (setWrite.setIndex >= sets_.size()) {
      VKR_PIPE_ERROR("Descriptor set write index {} out of range, count {}",
                     setWrite.setIndex, sets_.size());
    }

    for (const auto &bufferWrite : setWrite.buffers) {
      if (bufferWrite.buffers.empty()) {
        VKR_PIPE_ERROR("Descriptor buffer write for set {}, binding {} has no "
                       "buffer infos",
                       setWrite.setIndex, bufferWrite.binding);
      }

      VkWriteDescriptorSet write{};
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = sets_[setWrite.setIndex];
      write.dstBinding = bufferWrite.binding;
      write.dstArrayElement = bufferWrite.arrayElement;
      write.descriptorType = bufferWrite.type;
      write.descriptorCount = bufferWrite.descriptorCount();
      write.pBufferInfo = bufferWrite.buffers.data();

      writes.push_back(write);
    }

    for (const auto &imageWrite : setWrite.images) {
      if (imageWrite.images.empty()) {
        VKR_PIPE_ERROR(
            "Descriptor image write for set {}, binding {} has no image infos",
            setWrite.setIndex, imageWrite.binding);
      }

      VkWriteDescriptorSet write{};
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = sets_[setWrite.setIndex];
      write.dstBinding = imageWrite.binding;
      write.dstArrayElement = imageWrite.arrayElement;
      write.descriptorType = imageWrite.type;
      write.descriptorCount = imageWrite.descriptorCount();
      write.pImageInfo = imageWrite.images.data();

      writes.push_back(write);
    }
  }

  if (writes.empty()) {
    return;
  }

  vkUpdateDescriptorSets(device_.device(), static_cast<uint32_t>(writes.size()),
                         writes.data(), 0, nullptr);

  VKR_PIPE_INFO("Updated descriptor sets with {} writes", writes.size());
}

auto DescriptorSets::set(uint32_t index) const -> VkDescriptorSet {
  if (index >= sets_.size()) {
    VKR_PIPE_ERROR("Descriptor set index {} out of range, count {}", index,
                   sets_.size());
  }

  return sets_[index];
}

} // namespace vkr::pipeline
