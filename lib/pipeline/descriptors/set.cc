#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/logger.hh"
#include "vkr/pipeline/attachments/color.hh"
#include "vkr/pipeline/attachments/depth.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/resource/buffers/buffer.hh"
#include "vkr/resource/image/cubemap.hh"
#include "vkr/resource/image/texture.hh"
#include <algorithm>
#include <utility>

namespace vkr::pipe {

DescriptorSet::DescriptorSet(const core::Device &device,
                             const DescriptorPool &pool,
                             const DescriptorSetLayout &layout)
    : device_(device), pool_(pool), layout_(layout) {}

DescriptorSet::~DescriptorSet() { destroy(); }

auto DescriptorSet::texture(uint32_t binding, const res::Texture &texture)
    -> DescriptorSet & {
  assign(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         texture.descriptorInfo());
  return *this;
}

auto DescriptorSet::texture(uint32_t binding, const res::Cubemap &cubemap)
    -> DescriptorSet & {
  assign(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         cubemap.descriptorInfo());
  return *this;
}

auto DescriptorSet::texture(uint32_t binding, const ColorAttachment &attachment)
    -> DescriptorSet & {
  assign(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         attachment.descriptorInfo());
  return *this;
}

auto DescriptorSet::texture(uint32_t binding, const DepthAttachment &attachment)
    -> DescriptorSet & {
  assign(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         attachment.descriptorInfo());
  return *this;
}

auto DescriptorSet::uniform(uint32_t binding, const res::Buffer &buffer)
    -> DescriptorSet & {
  return uniform(binding, buffer.slice(0, buffer.size()));
}

auto DescriptorSet::uniform(uint32_t binding, const res::BufferSlice &slice)
    -> DescriptorSet & {
  if ((slice.usage() & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) == 0) {
    VKR_PIPE_ERROR("Uniform binding requires UNIFORM_BUFFER usage");
  }
  assign(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, slice.descriptorInfo());
  return *this;
}

auto DescriptorSet::storage(uint32_t binding, const res::Buffer &buffer)
    -> DescriptorSet & {
  return storage(binding, buffer.slice(0, buffer.size()));
}

auto DescriptorSet::storage(uint32_t binding, const res::BufferSlice &slice)
    -> DescriptorSet & {
  if ((slice.usage() & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) == 0) {
    VKR_PIPE_ERROR("Storage binding requires STORAGE_BUFFER usage");
  }
  assign(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, slice.descriptorInfo());
  return *this;
}

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
    validateBuffer(buffer.type, buffer.info);
  }
  for (const auto &image : images_) {
    validateBinding(image.binding, image.type);
    validateImage(image.info);
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
  validateBuffer(type, info);
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

void DescriptorSet::validateBuffer(VkDescriptorType type,
                                   const VkDescriptorBufferInfo &info) const {
  const bool uniform = type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  const auto &limits = device_.properties().limits;
  const auto alignment = uniform ? limits.minUniformBufferOffsetAlignment
                                 : limits.minStorageBufferOffsetAlignment;
  const auto maxRange =
      uniform ? limits.maxUniformBufferRange : limits.maxStorageBufferRange;
  if (info.buffer == VK_NULL_HANDLE || info.range == 0 ||
      info.range > maxRange || info.offset % alignment != 0) {
    VKR_PIPE_ERROR("Buffer descriptor requires a valid handle, an aligned "
                   "offset, and an explicit range within device limits");
  }
}

void DescriptorSet::assign(uint32_t binding, VkDescriptorType type,
                           VkDescriptorImageInfo info) {
  validateImage(info);
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

void DescriptorSet::validateImage(const VkDescriptorImageInfo &info) {
  if (info.imageView == VK_NULL_HANDLE || info.sampler == VK_NULL_HANDLE ||
      info.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
    VKR_PIPE_ERROR("Texture binding requires a valid image view, sampler, "
                   "and descriptor layout");
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
    validateBinding(buffer.binding, buffer.type);
    validateBuffer(buffer.type, buffer.info);
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
    validateBinding(image.binding, image.type);
    validateImage(image.info);
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

} // namespace vkr::pipe
