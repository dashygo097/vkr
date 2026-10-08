#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <utility>

namespace vkr::pipeline {

DescriptorSets::DescriptorSets(const core::Device &device,
                               const DescriptorPool &pool,
                               const DescriptorSetLayout &layout)
    : device_(device), pool_(pool), layout_(layout) {}

DescriptorSets::~DescriptorSets() { destroy(); }

void DescriptorSets::create() {
  destroy();

  if (desc_.setCount == 0) {
    bindings_.clear();
    ++binding_revision_;
    VKR_PIPE_TRACE("Descriptor set allocation skipped because setCount is 0");
    return;
  }

  if (!pool_.valid() || !layout_.valid()) {
    VKR_PIPE_ERROR(
        "Cannot allocate descriptor sets without a valid pool/layout");
  }
  if ((pool_.desc().flags &
       VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT) == 0) {
    VKR_PIPE_ERROR(
        "DescriptorSets requires a pool supporting individual frees");
  }

  auto nextBindings = bindings_;
  const auto &declarations = layout_.desc().bindings;
  nextBindings.erase(
      std::remove_if(
          nextBindings.begin(), nextBindings.end(),
          [&declarations](const Binding &binding) -> bool {
            return std::none_of(
                declarations.begin(), declarations.end(),
                [&binding](const DescriptorBinding &declaration) -> bool {
                  return declaration.layout.binding == binding.binding &&
                         declaration.layout.descriptorType == binding.type &&
                         declaration.layout.descriptorCount == 1;
                });
          }),
      nextBindings.end());

  allocateSets();
  bindings_ = std::move(nextBindings);
  ++binding_revision_;
}

void DescriptorSets::destroy() {
  if (!sets_.empty() && pool_.valid()) {
    vkFreeDescriptorSets(device_.device(), pool_.pool(),
                         static_cast<uint32_t>(sets_.size()), sets_.data());
  }

  sets_.clear();
  written_revisions_.clear();
  initialized_.clear();
}

void DescriptorSets::update(const DescriptorSetsDesc &desc) {
  desc_ = desc;
  create();
}

void DescriptorSets::allocateSets() {
  std::vector<VkDescriptorSetLayout> layouts(desc_.setCount, layout_.layout());

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool_.pool();
  allocInfo.descriptorSetCount = desc_.setCount;
  allocInfo.pSetLayouts = layouts.data();

  std::vector<VkDescriptorSet> sets(desc_.setCount, VK_NULL_HANDLE);
  const VkResult result =
      vkAllocateDescriptorSets(device_.device(), &allocInfo, sets.data());
  if (result != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to allocate descriptor sets. VkResult: {}",
                   static_cast<int>(result));
  }

  sets_ = std::move(sets);
  written_revisions_.assign(sets_.size(), 0);
  initialized_.assign(sets_.size(),
                      std::vector<uint8_t>(layout_.desc().bindings.size(), 0));

  VKR_PIPE_INFO("Allocated {} descriptor sets", sets_.size());
}

void DescriptorSets::write(const std::vector<DescriptorSetWrite> &setWrites) {
  apply(setWrites);

  for (const auto &setWrite : setWrites) {
    for (const auto &buffer : setWrite.buffers) {
      bindings_.erase(std::remove_if(bindings_.begin(), bindings_.end(),
                                     [&buffer](const Binding &binding) -> bool {
                                       return binding.binding == buffer.binding;
                                     }),
                      bindings_.end());
    }
    for (const auto &image : setWrite.images) {
      bindings_.erase(std::remove_if(bindings_.begin(), bindings_.end(),
                                     [&image](const Binding &binding) -> bool {
                                       return binding.binding == image.binding;
                                     }),
                      bindings_.end());
    }
  }
  if (!setWrites.empty()) {
    ++binding_revision_;
  }
}

void DescriptorSets::apply(const std::vector<DescriptorSetWrite> &setWrites) {
  if (setWrites.empty()) {
    return;
  }
  if (!valid()) {
    VKR_PIPE_ERROR("Cannot write descriptor sets before allocation");
  }

  std::vector<VkWriteDescriptorSet> writes{};
  size_t writeCount = 0;
  for (const auto &setWrite : setWrites) {
    writeCount += setWrite.buffers.size() + setWrite.images.size();
  }
  writes.reserve(writeCount);

  for (const auto &setWrite : setWrites) {
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

      (void)bindingIndex(bufferWrite.binding, bufferWrite.type,
                         bufferWrite.arrayElement,
                         bufferWrite.descriptorCount());

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

      (void)bindingIndex(imageWrite.binding, imageWrite.type,
                         imageWrite.arrayElement, imageWrite.descriptorCount());

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

  for (const auto &setWrite : setWrites) {
    written_revisions_[setWrite.setIndex] = 0;
    for (const auto &buffer : setWrite.buffers) {
      const auto index =
          bindingIndex(buffer.binding, buffer.type, buffer.arrayElement,
                       buffer.descriptorCount());
      if (buffer.arrayElement == 0 &&
          buffer.descriptorCount() ==
              layout_.desc().bindings[index].layout.descriptorCount) {
        initialized_[setWrite.setIndex][index] = 1;
      }
    }
    for (const auto &image : setWrite.images) {
      const auto index =
          bindingIndex(image.binding, image.type, image.arrayElement,
                       image.descriptorCount());
      if (image.arrayElement == 0 &&
          image.descriptorCount() ==
              layout_.desc().bindings[index].layout.descriptorCount) {
        initialized_[setWrite.setIndex][index] = 1;
      }
    }
  }
}

auto DescriptorSets::bindingIndex(uint32_t binding, VkDescriptorType type,
                                  uint32_t arrayElement, uint32_t count) const
    -> size_t {
  const auto &declarations = layout_.desc().bindings;
  for (size_t index = 0; index < declarations.size(); ++index) {
    const auto &declaration = declarations[index].layout;
    if (declaration.binding != binding) {
      continue;
    }
    if (declaration.descriptorType != type ||
        arrayElement > declaration.descriptorCount ||
        count > declaration.descriptorCount - arrayElement) {
      VKR_PIPE_ERROR("Descriptor write does not match binding {}", binding);
    }
    return index;
  }
  VKR_PIPE_ERROR("Descriptor binding {} is not declared", binding);
}

void DescriptorSets::bind(uint32_t binding, VkDescriptorType type,
                          std::variant<BufferInfo, ImageInfo> info) {
  if (layout_.valid()) {
    const auto index = bindingIndex(binding, type, 0, 1);
    if (layout_.desc().bindings[index].layout.descriptorCount != 1) {
      VKR_PIPE_ERROR("Resource binding {} requires a single descriptor",
                     binding);
    }
  }
  auto existing = std::find_if(bindings_.begin(), bindings_.end(),
                               [binding](const Binding &value) -> bool {
                                 return value.binding == binding;
                               });
  if (existing == bindings_.end()) {
    bindings_.push_back({binding, type, std::move(info)});
  } else {
    *existing = {binding, type, std::move(info)};
  }
  ++binding_revision_;
}

void DescriptorSets::write(uint32_t setIndex) {
  if (sets_.empty() && bindings_.empty()) {
    return;
  }
  if (setIndex >= sets_.size()) {
    VKR_PIPE_ERROR("Descriptor set index {} out of range", setIndex);
  }
  if (written_revisions_[setIndex] == binding_revision_) {
    return;
  }

  auto write = DescriptorSetWrite::forSet(setIndex);
  for (const auto &binding : bindings_) {
    const auto index = bindingIndex(binding.binding, binding.type, 0, 1);
    if (layout_.desc().bindings[index].layout.descriptorCount != 1) {
      VKR_PIPE_ERROR("Resource binding {} requires a single descriptor",
                     binding.binding);
    }
    if (std::holds_alternative<BufferInfo>(binding.info)) {
      const auto info = std::get<BufferInfo>(binding.info)(setIndex);
      if (info.buffer == VK_NULL_HANDLE || info.range == 0) {
        VKR_PIPE_ERROR("Resource binding {} has no valid buffer",
                       binding.binding);
      }
      write.buffers.push_back(
          DescriptorBufferWrite::one(binding.binding, binding.type, info));
    } else {
      const auto info = std::get<ImageInfo>(binding.info)(setIndex);
      if (info.imageView == VK_NULL_HANDLE || info.sampler == VK_NULL_HANDLE ||
          info.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        VKR_PIPE_ERROR("Resource binding {} has no valid sampled image",
                       binding.binding);
      }
      write.images.push_back(
          DescriptorImageWrite::one(binding.binding, binding.type, info));
    }
  }
  for (size_t index = 0; index < initialized_[setIndex].size(); ++index) {
    const auto binding = layout_.desc().bindings[index].layout.binding;
    if (!initialized_[setIndex][index] &&
        std::none_of(bindings_.begin(), bindings_.end(),
                     [binding](const Binding &value) -> bool {
                       return value.binding == binding;
                     })) {
      VKR_PIPE_ERROR("Descriptor binding {} has no resource", binding);
    }
  }
  std::vector<DescriptorSetWrite> writes{};
  writes.push_back(std::move(write));
  apply(writes);
  written_revisions_[setIndex] = binding_revision_;
}

auto DescriptorSets::set(uint32_t index) const -> VkDescriptorSet {
  if (index >= sets_.size()) {
    VKR_PIPE_ERROR("Descriptor set index {} out of range, count {}", index,
                   sets_.size());
  }

  return sets_[index];
}

} // namespace vkr::pipeline
