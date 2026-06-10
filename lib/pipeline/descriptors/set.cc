#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/logger.hh"

namespace vkr::pipeline {

DescriptorSets::DescriptorSets(const core::Device &device,
                               resource::ResourceManager &ResourceManager,
                               DescriptorSetLayout &layout,
                               const DescriptorPool &pool, uint32_t frameCount)
    : device_(device), resource_manager_(ResourceManager), layout_(layout),
      pool_(pool), frame_count_(frameCount) {
  allocateSets();
}

DescriptorSets::~DescriptorSets() {
  if (!sets_.empty()) {
    vkFreeDescriptorSets(device_.device(), pool_.pool(),
                         static_cast<uint32_t>(sets_.size()), sets_.data());
  }
}

void DescriptorSets::allocateSets() {
  std::vector<VkDescriptorSetLayout> layouts(frame_count_, layout_.layoutRef());

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool_.pool();
  allocInfo.descriptorSetCount = frame_count_;
  allocInfo.pSetLayouts = layouts.data();

  sets_.resize(frame_count_);
  VkResult result =
      vkAllocateDescriptorSets(device_.device(), &allocInfo, sets_.data());
  if (result != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to allocate descriptor sets. VkResult: {}",
                   std::to_string(result));
  }
}

void DescriptorSets::autoBindResources() {
  for (const auto &binding : layout_.bindings()) {
    if (binding.name.empty()) {
      VKR_PIPE_ERROR("Descriptor binding {} has empty name", binding.binding);
    }

    switch (binding.type) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
      auto uniformBuffer = resource_manager_.getUniformBuffer(binding.name);

      if (!uniformBuffer) {
        VKR_PIPE_ERROR("Uniform buffer resource not found: {}", binding.name);
      }

      bindUniformBuffer(binding.binding, uniformBuffer->buffers(),
                        uniformBuffer->bufferSize());
      break;
    }

    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
      auto imageView = resource_manager_.getTextureImageView(binding.name);
      auto sampler = resource_manager_.getTextureSampler(binding.name);

      if (!imageView) {
        VKR_PIPE_ERROR("Texture image view resource not found: {}",
                       binding.name);
      }

      if (!sampler) {
        VKR_PIPE_ERROR("Texture sampler resource not found: {}", binding.name);
      }

      std::vector<VkImageView> imageViews(frame_count_, imageView->imageView());
      bindImageSampler(binding.binding, imageViews, sampler->sampler());
      break;
    }

    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      VKR_PIPE_ERROR("Auto bind for storage buffer is not implemented: {}",
                     binding.name);
      break;

    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      VKR_PIPE_ERROR("Auto bind for storage image is not implemented: {}",
                     binding.name);
      break;

    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      VKR_PIPE_ERROR("Auto bind for input attachment is not implemented: {}",
                     binding.name);
      break;

    default:
      VKR_PIPE_ERROR("Unknown descriptor type for binding {}", binding.binding);
      break;
    }
  }
}

void DescriptorSets::bindUniformBuffer(uint32_t binding,
                                       const std::vector<VkBuffer> &buffers,
                                       VkDeviceSize size, VkDeviceSize offset) {
  if (buffers.size() != frame_count_) {
    VKR_PIPE_ERROR("Buffer count must match frame count({} vs {})",
                   buffers.size(), frame_count_);
  }

  for (uint32_t i = 0; i < frame_count_; ++i) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffers[i];
    bufferInfo.offset = offset;
    bufferInfo.range = size;

    DescriptorWriter writer(device_);
    writer.writeBuffer(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
    writer.update(sets_[i]);
  }
}

void DescriptorSets::bindStorageBuffer(uint32_t binding,
                                       const std::vector<VkBuffer> &buffers,
                                       VkDeviceSize size, VkDeviceSize offset) {
  if (buffers.size() != frame_count_) {
    VKR_PIPE_ERROR("Buffer count must match frame count({} vs {})",
                   buffers.size(), frame_count_);
  }

  for (uint32_t i = 0; i < frame_count_; ++i) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffers[i];
    bufferInfo.offset = offset;
    bufferInfo.range = size;

    DescriptorWriter writer(device_);
    writer.writeBuffer(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo);
    writer.update(sets_[i]);
  }
}

void DescriptorSets::bindImageSampler(
    uint32_t binding, const std::vector<VkImageView> &imageViews,
    VkSampler sampler, VkImageLayout layout) {
  if (imageViews.size() != frame_count_) {
    VKR_PIPE_ERROR("Image view count must match frame count({} vs {})",
                   imageViews.size(), frame_count_);
  }

  for (uint32_t i = 0; i < frame_count_; ++i) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = layout;
    imageInfo.imageView = imageViews[i];
    imageInfo.sampler = sampler;

    DescriptorWriter writer(device_);
    writer.writeImage(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      &imageInfo);
    writer.update(sets_[i]);
  }
}

void DescriptorSets::bindToFrame(uint32_t frameIndex,
                                 DescriptorWriter &writer) {
  if (frameIndex >= frame_count_) {
    VKR_PIPE_ERROR("Frame index out of bounds: {}", frameIndex);
  }
  writer.update(sets_[frameIndex]);
}

void DescriptorSets::bind(VkCommandBuffer cmd, VkPipelineLayout layout,
                          uint32_t frameIndex, VkPipelineBindPoint bindPoint) {
  if (frameIndex >= frame_count_) {
    VKR_PIPE_ERROR("Frame index out of bounds: {}", frameIndex);
  }
  vkCmdBindDescriptorSets(cmd, bindPoint, layout, 0, 1, &sets_[frameIndex], 0,
                          nullptr);
}

} // namespace vkr::pipeline
