#include "vkr/exec/compute/passes/compute.hh"
#include "vkr/logger.hh"
#include <algorithm>

namespace vkr::exec {

ComputePass::ComputePass(ComputeExecutor &executor, const core::Device &device)
    : executor_(executor), device_(device), descriptor_pool_(device) {}

ComputePass::~ComputePass() { destroy(); }

void ComputePass::create() {
  validate(desc_);
  destroy();
  createDescriptors();
  createPipeline();
}

void ComputePass::destroy() noexcept {
  pipeline_.reset();
  for (auto &set : descriptor_sets_) {
    set.destroy();
  }
  for (auto &layout : descriptor_layouts_) {
    layout->destroy();
  }
  descriptor_pool_.destroy();
}

void ComputePass::update(const ComputePassDesc &desc) {
  ensureConfigurable();
  if (pipeline_ || descriptor_pool_.valid()) {
    VKR_EXEC_ERROR("ComputePass '{}' must be destroyed before updating its "
                   "configuration",
                   name());
  }
  validate(desc);
  auto nextDesc = desc;
  const auto setCount = std::max<size_t>(1, nextDesc.descriptorLayouts.size());
  while (descriptor_layouts_.size() > setCount) {
    descriptor_sets_.pop_back();
    descriptor_layouts_.pop_back();
  }
  while (descriptor_layouts_.size() < setCount) {
    descriptor_layouts_.push_back(
        std::make_unique<pipeline::DescriptorSetLayout>(device_));
    descriptor_sets_.emplace_back(device_, descriptor_pool_,
                                  *descriptor_layouts_.back());
  }
  desc_ = std::move(nextDesc);
}

void ComputePass::record() {
  if (!pipeline_ || !pipeline_->valid()) {
    VKR_EXEC_ERROR("ComputePass '{}' recorded without a valid compute "
                   "pipeline",
                   name());
  }

  if (!desc_.dispatch.isValid()) {
    VKR_EXEC_ERROR("ComputePass '{}' has invalid dispatch group count", name());
  }

  executor_.beginProfileScope(name());
  if (!descriptor_layouts_.empty() && descriptor_layouts_.front()->valid()) {
    executor_.bindPipeline(*pipeline_, descriptor_sets_);
  } else {
    executor_.bindPipeline(*pipeline_);
  }
  executor_.dispatch(desc_.dispatch.groupCountX, desc_.dispatch.groupCountY,
                     desc_.dispatch.groupCountZ);
  executor_.endProfileScope();
}

void ComputePass::validate(const ComputePassDesc &desc) const {
  if (!desc.pipeline.isValid()) {
    VKR_EXEC_ERROR("ComputePass '{}' has an invalid pipeline descriptor",
                   name());
  }
  if (!desc.dispatch.isValid()) {
    VKR_EXEC_ERROR("ComputePass '{}' has invalid dispatch group counts",
                   name());
  }
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device_.physicalDevice(), &properties);
  auto layouts = desc.descriptorLayouts;
  if (layouts.size() > properties.limits.maxBoundDescriptorSets) {
    VKR_EXEC_ERROR("ComputePass '{}' exceeds maxBoundDescriptorSets", name());
  }
  for (const auto &layoutDesc : layouts) {
    if (!layoutDesc.isValid()) {
      VKR_EXEC_ERROR("ComputePass '{}' requires unique bindings within each set "
                     "with nonzero counts/stages", name());
    }
    for (const auto &binding : layoutDesc.bindings) {
      const auto &layout = binding.layout;
      if (layout.descriptorCount != 1 ||
          (layout.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
           layout.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
           layout.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) {
        VKR_EXEC_ERROR("ComputePass '{}' has an unsupported descriptor binding {}",
                       name(), layout.binding);
      }
    }
  }
}

void ComputePass::createDescriptors() {
  auto layouts = desc_.descriptorLayouts;
  if (layouts.empty()) {
    return;
  }
  descriptor_pool_.update(pipeline::DescriptorPoolDesc::sets(layouts, 1));
  for (uint32_t setIndex = 0; setIndex < layouts.size(); ++setIndex) {
    descriptor_layouts_[setIndex]->update(layouts[setIndex]);
    descriptor_sets_[setIndex].create();
  }
}

void ComputePass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;

  if (pipelineDesc.layout.setLayouts.empty()) {
    for (const auto &layout : descriptor_layouts_) {
      if (layout->valid()) {
        pipelineDesc.layout.setLayouts.push_back(layout->layout());
      }
    }
  }

  pipeline_ = std::make_unique<pipeline::ComputePipeline>(device_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR("ComputePass '{}' failed to create compute pipeline '{}'",
                   name(), pipelineDesc.name);
  }
}

} // namespace vkr::exec
