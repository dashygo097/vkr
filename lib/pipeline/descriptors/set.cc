#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <utility>

namespace vkr::pipeline {

DescriptorSet::DescriptorSet(const core::Device &device,
                             const DescriptorPool &pool,
                             const DescriptorSetLayout &layout)
    : device_(device), pool_(pool), layout_(layout) {}

DescriptorSet::~DescriptorSet() { destroy(); }

DescriptorSet::DescriptorSet(DescriptorSet &&other) noexcept
    : device_(other.device_), pool_(other.pool_), layout_(other.layout_),
      set_(std::exchange(other.set_, VK_NULL_HANDLE)),
      buffers_(std::move(other.buffers_)), images_(std::move(other.images_)),
      writes_(std::move(other.writes_)) {
  writes_.clear();
}

void DescriptorSet::create() {
  destroy();
  if (!pool_.valid() || !layout_.valid()) {
    VKR_PIPE_ERROR(
        "Cannot allocate a descriptor set without a valid pool/layout");
  }
  if ((pool_.desc().flags &
       VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT) == 0) {
    VKR_PIPE_ERROR("DescriptorSet requires a pool supporting individual frees");
  }

  for (const auto &buffer : buffers_) {
    validateBinding(buffer.binding, buffer.type);
  }
  for (const auto &image : images_) {
    validateBinding(image.binding, image.type);
  }

  const auto layout = layout_.layout();
  VkDescriptorSetAllocateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info.descriptorPool = pool_.pool();
  info.descriptorSetCount = 1;
  info.pSetLayouts = &layout;
  const auto result = vkAllocateDescriptorSets(device_.device(), &info, &set_);
  if (result != VK_SUCCESS) {
    set_ = VK_NULL_HANDLE;
    VKR_PIPE_ERROR("Failed to allocate a descriptor set. VkResult: {}",
                   static_cast<int>(result));
  }
}

void DescriptorSet::destroy() noexcept {
  if (valid() && pool_.valid()) {
    vkFreeDescriptorSets(device_.device(), pool_.pool(), 1, &set_);
  }
  set_ = VK_NULL_HANDLE;
  writes_.clear();
  for (auto &buffer : buffers_) {
    buffer.dirty = true;
  }
  for (auto &image : images_) {
    image.dirty = true;
  }
}

void DescriptorSet::validateBinding(uint32_t binding,
                                    VkDescriptorType type) const {
  const auto &declarations = layout_.desc().bindings;
  const auto declaration =
      std::find_if(declarations.begin(), declarations.end(),
                   [binding](const DescriptorBinding &value) -> bool {
                     return value.layout.binding == binding;
                   });
  if (declaration == declarations.end() ||
      declaration->layout.descriptorType != type ||
      declaration->layout.descriptorCount != 1) {
    VKR_PIPE_ERROR("Resource does not match descriptor binding {}", binding);
  }
}

void DescriptorSet::assign(uint32_t binding, VkDescriptorType type,
                           VkDescriptorBufferInfo info) {
  if (layout_.valid()) {
    validateBinding(binding, type);
  }
  images_.erase(std::remove_if(images_.begin(), images_.end(),
                               [binding](const auto &image) -> bool {
                                 return image.binding == binding;
                               }),
                images_.end());
  const auto existing = std::find_if(buffers_.begin(), buffers_.end(),
                                     [binding](const auto &buffer) -> bool {
                                       return buffer.binding == binding;
                                     });
  if (existing == buffers_.end()) {
    buffers_.push_back({binding, type, info});
    writes_.reserve(buffers_.size() + images_.size());
  } else {
    *existing = {binding, type, info};
  }
}

void DescriptorSet::assign(uint32_t binding, VkDescriptorType type,
                           VkDescriptorImageInfo info) {
  if (layout_.valid()) {
    validateBinding(binding, type);
  }
  buffers_.erase(std::remove_if(buffers_.begin(), buffers_.end(),
                                [binding](const auto &buffer) -> bool {
                                  return buffer.binding == binding;
                                }),
                 buffers_.end());
  const auto existing = std::find_if(images_.begin(), images_.end(),
                                     [binding](const auto &image) -> bool {
                                       return image.binding == binding;
                                     });
  if (existing == images_.end()) {
    images_.push_back({binding, type, info});
    writes_.reserve(buffers_.size() + images_.size());
  } else {
    *existing = {binding, type, info};
  }
}

void DescriptorSet::update() {
  if (!valid() || !pool_.valid() || !layout_.valid()) {
    VKR_PIPE_ERROR("Cannot update a descriptor set without a valid allocation");
  }

  if (buffers_.size() + images_.size() != layout_.desc().bindings.size()) {
    for (const auto &declaration : layout_.desc().bindings) {
      const auto binding = declaration.layout.binding;
      const auto buffer = std::find_if(buffers_.begin(), buffers_.end(),
                                       [binding](const auto &value) -> bool {
                                         return value.binding == binding;
                                       });
      const auto image = std::find_if(images_.begin(), images_.end(),
                                      [binding](const auto &value) -> bool {
                                        return value.binding == binding;
                                      });
      if (buffer == buffers_.end() && image == images_.end()) {
        VKR_PIPE_ERROR("Descriptor binding {} has no resource", binding);
      }
    }
  }

  writes_.clear();
  for (const auto &buffer : buffers_) {
    if (!buffer.dirty) {
      continue;
    }
    if (buffer.info.buffer == VK_NULL_HANDLE || buffer.info.range == 0) {
      VKR_PIPE_ERROR("Descriptor binding {} has no valid buffer",
                     buffer.binding);
    }
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set_;
    write.dstBinding = buffer.binding;
    write.descriptorType = buffer.type;
    write.descriptorCount = 1;
    write.pBufferInfo = &buffer.info;
    writes_.push_back(write);
  }
  for (const auto &image : images_) {
    if (!image.dirty) {
      continue;
    }
    if (image.info.imageView == VK_NULL_HANDLE ||
        image.info.sampler == VK_NULL_HANDLE ||
        image.info.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
      VKR_PIPE_ERROR("Descriptor binding {} has no valid sampled image",
                     image.binding);
    }
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set_;
    write.dstBinding = image.binding;
    write.descriptorType = image.type;
    write.descriptorCount = 1;
    write.pImageInfo = &image.info;
    writes_.push_back(write);
  }

  if (!writes_.empty()) {
    vkUpdateDescriptorSets(device_.device(),
                           static_cast<uint32_t>(writes_.size()),
                           writes_.data(), 0, nullptr);
    for (auto &buffer : buffers_) {
      buffer.dirty = false;
    }
    for (auto &image : images_) {
      image.dirty = false;
    }
  }
}

} // namespace vkr::pipeline
