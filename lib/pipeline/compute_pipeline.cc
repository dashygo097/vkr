#include "vkr/pipeline/compute_pipeline.hh"
#include "vkr/logger.hh"

namespace vkr::pipeline {

ComputePipeline::ComputePipeline(const core::Device &device) : device_(device) {
}

ComputePipeline::~ComputePipeline() { destroy(); }

void ComputePipeline::destroy() {
  if (vk_compute_pipeline_ != VK_NULL_HANDLE ||
      vk_pipeline_layout_ != VK_NULL_HANDLE || !retired_pipelines_.empty()) {
    vkDeviceWaitIdle(device_.device());
  }

  if (vk_compute_pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_.device(), vk_compute_pipeline_, nullptr);
    vk_compute_pipeline_ = VK_NULL_HANDLE;
  }

  if (vk_pipeline_layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_.device(), vk_pipeline_layout_, nullptr);
    vk_pipeline_layout_ = VK_NULL_HANDLE;
  }

  for (const auto &retired : retired_pipelines_) {
    if (retired.pipeline != VK_NULL_HANDLE) {
      vkDestroyPipeline(device_.device(), retired.pipeline, nullptr);
    }

    if (retired.layout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(device_.device(), retired.layout, nullptr);
    }
  }

  retired_pipelines_.clear();
  shader_module_.reset();
}

auto ComputePipeline::update(const ComputePipelineDesc &desc) -> bool {
  desc_ = desc;

  if (!desc_.isValid()) {
    VKR_PIPE_ERROR("Invalid compute pipeline descriptor");
  }

  auto shaderDesc = desc_.shader;
  shaderDesc.setEntryPoint(desc_.entryPoint);

  auto nextShaderModule = std::make_unique<resource::ShaderModule>(device_);
  nextShaderModule->update(shaderDesc);

  if (!nextShaderModule->valid()) {
    VKR_PIPE_ERROR("Failed to create compute shader module for pipeline '{}'",
                   desc_.name);
  }

  VkPipelineShaderStageCreateInfo shaderStage{};
  shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStage.module = nextShaderModule->module();
  shaderStage.pName = desc_.entryPoint.c_str();

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount =
      static_cast<uint32_t>(desc_.layout.setLayouts.size());
  layoutInfo.pSetLayouts = desc_.layout.setLayouts.empty()
                               ? nullptr
                               : desc_.layout.setLayouts.data();
  layoutInfo.pushConstantRangeCount =
      static_cast<uint32_t>(desc_.layout.pushConstants.size());
  layoutInfo.pPushConstantRanges = desc_.layout.pushConstants.empty()
                                       ? nullptr
                                       : desc_.layout.pushConstants.data();

  VkPipelineLayout nextLayout{VK_NULL_HANDLE};
  if (vkCreatePipelineLayout(device_.device(), &layoutInfo, nullptr,
                             &nextLayout) != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create compute pipeline layout for '{}'",
                   desc_.name);
  }

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.stage = shaderStage;
  pipelineInfo.layout = nextLayout;
  pipelineInfo.basePipelineHandle = desc_.basePipeline;
  pipelineInfo.basePipelineIndex = desc_.basePipelineIndex;

  VkPipeline nextPipeline{VK_NULL_HANDLE};
  if (vkCreateComputePipelines(device_.device(), VK_NULL_HANDLE, 1,
                               &pipelineInfo, nullptr,
                               &nextPipeline) != VK_SUCCESS) {
    vkDestroyPipelineLayout(device_.device(), nextLayout, nullptr);
    VKR_PIPE_ERROR("Failed to create compute pipeline '{}'", desc_.name);
  }

  if (vk_compute_pipeline_ != VK_NULL_HANDLE ||
      vk_pipeline_layout_ != VK_NULL_HANDLE) {
    RetiredPipeline retired{};
    retired.pipeline = vk_compute_pipeline_;
    retired.layout = vk_pipeline_layout_;
    retired_pipelines_.push_back(retired);
  }

  shader_module_ = std::move(nextShaderModule);
  vk_pipeline_layout_ = nextLayout;
  vk_compute_pipeline_ = nextPipeline;
  ++revision_;

  VKR_PIPE_INFO("Compute pipeline '{}' created", desc_.name);
  return true;
}

} // namespace vkr::pipeline
