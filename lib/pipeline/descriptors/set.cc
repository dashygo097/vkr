#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/logger.hh"

namespace vkr::pipeline {

DescriptorSets::DescriptorSets(const core::Device &device,
                               const resource::ResourceManager &ResourceManager,
                               const DescriptorPool &pool,
                               DescriptorSetLayout &layout, uint32_t frameCount)
    : device_(device), resource_manager_(ResourceManager), pool_(pool),
      layout_(layout), frame_count_(frameCount) {
  allocateSets();
}

DescriptorSets::~DescriptorSets() {
  if (!sets_.empty()) {
    vkFreeDescriptorSets(device_.device(), pool_.pool(),
                         static_cast<uint32_t>(sets_.size()), sets_.data());
  }
}

void DescriptorSets::allocateSets() {
  std::vector<VkDescriptorSetLayout> layouts(frame_count_, layout_.layout());

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

void DescriptorSets::bindResources() {
  for (const auto &binding : layout_.bindings()) {
    if (binding.name.empty()) {
      VKR_PIPE_ERROR("Descriptor binding {} has empty name",
                     binding.layout.binding);
    }

    switch (binding.layout.descriptorType) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
      auto uniformBuffer = resource_manager_.getUniformBuffer(binding.name);

      if (!uniformBuffer) {
        VKR_PIPE_ERROR("Uniform buffer resource not found: {}", binding.name);
      }

      const auto &buffers = uniformBuffer->buffers();
      const auto size = uniformBuffer->bufferSize();

      if (buffers.size() != frame_count_) {
        VKR_PIPE_ERROR("Buffer count must match frame count({} vs {})",
                       buffers.size(), frame_count_);
      }
      for (uint32_t i = 0; i < frame_count_; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = size;

        DescriptorWriter writer(device_);
        writer.writeBuffer(binding.layout.binding,
                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
        writer.update(sets_[i]);
      }
      break;
    }

    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
      auto texture = resource_manager_.getTexture(binding.name);

      if (!texture) {
        VKR_PIPE_ERROR("Texture resource not found: {}", binding.name);
      }

      if (!texture->hasSampler()) {
        VKR_PIPE_ERROR("Texture sampler not found: {}", binding.name);
      }

      for (uint32_t i = 0; i < frame_count_; ++i) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texture->imageView();
        imageInfo.sampler = texture->sampler();

        DescriptorWriter writer(device_);
        writer.writeImage(binding.layout.binding,
                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                          &imageInfo);
        writer.update(sets_[i]);
      }
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
      VKR_PIPE_ERROR("Unknown descriptor type for binding {}",
                     binding.layout.binding);
      break;
    }
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
