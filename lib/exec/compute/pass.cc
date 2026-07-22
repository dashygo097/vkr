#include "vkr/exec/compute/pass.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <string_view>

namespace vkr::exec {
namespace {

void addPoolSize(pipeline::DescriptorPoolDesc &poolDesc,
                 VkDescriptorType type, uint32_t count) {
  if (count == 0) {
    return;
  }

  auto existing =
      std::find_if(poolDesc.poolSizes.begin(), poolDesc.poolSizes.end(),
                   [type](const VkDescriptorPoolSize &poolSize) {
                     return poolSize.type == type;
                   });

  if (existing != poolDesc.poolSizes.end()) {
    existing->descriptorCount += count;
    return;
  }

  poolDesc.poolSizes.push_back({type, count});
}

void validateUniqueDescriptorBindings(
    std::string_view passName,
    const std::vector<pipeline::DescriptorBinding> &bindings) {
  for (size_t i = 0; i < bindings.size(); ++i) {
    for (size_t j = i + 1; j < bindings.size(); ++j) {
      if (bindings[i].layout.binding == bindings[j].layout.binding) {
        VKR_EXEC_ERROR("ComputePass '{}' has duplicate descriptor binding {}",
                       std::string(passName), bindings[i].layout.binding);
      }
    }
  }
}

} // namespace

ComputePass::ComputePass(ComputeExecutor &executor, const core::Device &device)
    : executor_(executor), device_(device) {}

ComputePass::~ComputePass() { destroy(); }

void ComputePass::create() {
  destroy();
  createDescriptors();
  createPipeline();
}

void ComputePass::destroy() {
  pipeline_.reset();
  descriptor_sets_.reset();
  descriptor_layout_.reset();
  descriptor_pool_.reset();
}

void ComputePass::update(const ComputePassDesc &desc) { desc_ = desc; }

void ComputePass::record() {
  if (!pipeline_ || !pipeline_->valid()) {
    VKR_EXEC_ERROR("ComputePass '{}' recorded without a valid compute "
                   "pipeline",
                   name());
  }

  if (!desc_.dispatch.isValid()) {
    VKR_EXEC_ERROR("ComputePass '{}' has invalid dispatch group count",
                   name());
  }

  const std::vector<VkDescriptorSet> emptySets{};
  const auto &sets = descriptor_sets_ ? descriptor_sets_->sets() : emptySets;

  executor_.beginProfileScope(name());
  executor_.bindComputePipeline(pipeline_->pipeline(), pipeline_->layout(),
                                sets);
  executor_.dispatch(desc_.dispatch.groupCountX, desc_.dispatch.groupCountY,
                     desc_.dispatch.groupCountZ);
  executor_.endProfileScope();
}

void ComputePass::createDescriptors() {
  if (desc_.descriptorBindings.empty()) {
    if (!desc_.descriptorWrites.empty()) {
      VKR_EXEC_ERROR("ComputePass '{}' has descriptor writes but no "
                     "descriptor bindings",
                     name());
    }

    return;
  }

  const uint32_t setCount = descriptorSetCount();
  validateDescriptorWrites(setCount);

  auto bindings = descriptorBindings();
  validateUniqueDescriptorBindings(name(), bindings);

  descriptor_pool_ = std::make_unique<pipeline::DescriptorPool>(device_);
  descriptor_pool_->update(descriptorPoolDesc(setCount));

  descriptor_layout_ = std::make_unique<pipeline::DescriptorSetLayout>(device_);
  descriptor_layout_->update(
      pipeline::DescriptorSetLayoutDesc{.bindings = bindings});

  descriptor_sets_ = std::make_unique<pipeline::DescriptorSets>(device_);
  descriptor_sets_->update(pipeline::DescriptorSetsDesc{
      .pool = descriptor_pool_->pool(),
      .layout = descriptor_layout_->layout(),
      .setCount = setCount,
      .writes = desc_.descriptorWrites,
  });
}

void ComputePass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;

  const VkDescriptorSetLayout descriptorSetLayout =
      descriptor_layout_ ? descriptor_layout_->layout() : VK_NULL_HANDLE;
  if (descriptorSetLayout != VK_NULL_HANDLE &&
      pipelineDesc.layout.setLayouts.empty()) {
    pipelineDesc.layout.setLayouts = {descriptorSetLayout};
  }

  if (!pipelineDesc.isValid()) {
    VKR_EXEC_WARN("ComputePass '{}' has no valid compute pipeline desc",
                  name());
    return;
  }

  pipeline_ = std::make_unique<pipeline::ComputePipeline>(device_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR("ComputePass '{}' failed to create compute pipeline '{}'",
                   name(), pipelineDesc.name);
  }
}

auto ComputePass::descriptorSetCount() const -> uint32_t {
  uint32_t setCount = desc_.descriptorSetCount;

  for (const auto &write : desc_.descriptorWrites) {
    setCount = std::max(setCount, write.setIndex + 1);
  }

  if (setCount == 0) {
    VKR_EXEC_ERROR("ComputePass '{}' has zero descriptor sets", name());
  }

  return setCount;
}

auto ComputePass::descriptorPoolDesc(uint32_t setCount) const
    -> pipeline::DescriptorPoolDesc {
  auto poolDesc = desc_.descriptorPool;
  if (poolDesc.maxSets != 0) {
    return poolDesc;
  }

  for (const auto &binding : desc_.descriptorBindings) {
    const uint32_t descriptorCount = binding.layout.descriptorCount == 0
                                         ? 1U
                                         : binding.layout.descriptorCount;
    addPoolSize(poolDesc, binding.layout.descriptorType,
                descriptorCount * setCount);
  }

  poolDesc.maxSets = setCount;
  return poolDesc;
}

auto ComputePass::descriptorBindings() const
    -> std::vector<pipeline::DescriptorBinding> {
  auto bindings = desc_.descriptorBindings;

  for (auto &binding : bindings) {
    if (binding.layout.descriptorCount == 0) {
      binding.layout.descriptorCount = 1;
    }

    if (binding.layout.stageFlags == 0) {
      binding.layout.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }
  }

  return bindings;
}

void ComputePass::validateDescriptorWrites(uint32_t setCount) const {
  for (const auto &write : desc_.descriptorWrites) {
    if (write.setIndex >= setCount) {
      VKR_EXEC_ERROR("ComputePass '{}' descriptor write set index {} out of "
                     "range, set count {}",
                     name(), write.setIndex, setCount);
    }
  }
}

} // namespace vkr::exec
