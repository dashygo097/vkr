#include "vkr/exec/compute/passes/compute.hh"
#include "vkr/logger.hh"

namespace vkr::exec {

ComputePass::ComputePass(ComputeExecutor &executor, const core::Device &device)
    : executor_(executor), device_(device), descriptor_pool_(device),
      descriptor_layout_(device),
      descriptor_sets_(device, descriptor_pool_, descriptor_layout_) {}

ComputePass::~ComputePass() { destroy(); }

void ComputePass::create() {
  validate(desc_);
  destroy();
  createDescriptors();
  createPipeline();
}

void ComputePass::destroy() noexcept {
  pipeline_.reset();
  descriptor_sets_.destroy();
  descriptor_layout_.destroy();
  descriptor_pool_.destroy();
}

void ComputePass::update(const ComputePassDesc &desc) {
  ensureConfigurable();
  if (pipeline_ || descriptor_layout_.valid() || descriptor_pool_.valid()) {
    VKR_EXEC_ERROR("ComputePass '{}' must be destroyed before updating its "
                   "configuration",
                   name());
  }
  validate(desc);
  auto nextDesc = desc;
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
  descriptor_sets_.write(uint32_t{0});
  if (descriptor_sets_.valid()) {
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
  pipeline::DescriptorSetLayoutDesc layoutDesc{.bindings =
                                                   desc.descriptorBindings};
  if (!layoutDesc.isValid()) {
    VKR_EXEC_ERROR("ComputePass '{}' requires unique bindings with nonzero "
                   "counts/stages",
                   name());
  }
  for (const auto &binding : layoutDesc.bindings) {
    const auto &layout = binding.layout;
    if (layout.descriptorCount != 1 ||
        (layout.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
         layout.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
         layout.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) {
      VKR_EXEC_ERROR(
          "ComputePass '{}' has an unsupported descriptor binding {}", name(),
          layout.binding);
    }
  }
}

void ComputePass::createDescriptors() {
  if (desc_.descriptorBindings.empty()) {
    descriptor_sets_.update({.setCount = 0});
    return;
  }
  const pipeline::DescriptorSetLayoutDesc layoutDesc{
      .bindings = desc_.descriptorBindings};
  descriptor_pool_.update(pipeline::DescriptorPoolDesc::sets(layoutDesc, 1));
  descriptor_layout_.update(layoutDesc);
  descriptor_sets_.update({.setCount = 1});
  descriptor_sets_.write(uint32_t{0});
}

void ComputePass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;

  const VkDescriptorSetLayout descriptorSetLayout =
      descriptor_layout_.valid() ? descriptor_layout_.layout() : VK_NULL_HANDLE;
  if (descriptorSetLayout != VK_NULL_HANDLE &&
      pipelineDesc.layout.setLayouts.empty()) {
    pipelineDesc.layout.setLayouts = {descriptorSetLayout};
  }

  pipeline_ = std::make_unique<pipeline::ComputePipeline>(device_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR("ComputePass '{}' failed to create compute pipeline '{}'",
                   name(), pipelineDesc.name);
  }
}

} // namespace vkr::exec
