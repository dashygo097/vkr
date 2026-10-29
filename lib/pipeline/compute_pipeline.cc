#include "vkr/pipeline/compute_pipeline.hh"
#include "vkr/logger.hh"

namespace vkr::pipe {

ComputePipeline::ComputePipeline(const core::Device &device)
    : device_(device) {}

ComputePipeline::~ComputePipeline() { destroy(); }

ComputePipeline::Version::~Version() {
  if (pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_.device(), pipeline_, nullptr);
  }
  if (layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_.device(), layout_, nullptr);
  }
}

void ComputePipeline::destroy() {
  version_.reset();
  shader_module_.reset();
}

auto ComputePipeline::update(const ComputePipelineDesc &desc) -> bool {
  auto nextDesc = desc;

  if (!nextDesc.isValid()) {
    VKR_PIPE_ERROR("Invalid compute pipeline descriptor");
  }

  auto shaderDesc = nextDesc.shader;
  shaderDesc.setEntryPoint(nextDesc.entryPoint);

  auto nextShaderModule = std::make_unique<res::ShaderModule>(device_);
  nextShaderModule->update(shaderDesc);

  if (!nextShaderModule->valid()) {
    VKR_PIPE_ERROR("Failed to create compute shader module for pipeline '{}'",
                   nextDesc.name);
  }

  VkPipelineShaderStageCreateInfo shaderStage{};
  shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStage.module = nextShaderModule->module();
  shaderStage.pName = nextDesc.entryPoint.c_str();

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount =
      static_cast<uint32_t>(nextDesc.layout.setLayouts.size());
  layoutInfo.pSetLayouts = nextDesc.layout.setLayouts.empty()
                               ? nullptr
                               : nextDesc.layout.setLayouts.data();
  layoutInfo.pushConstantRangeCount =
      static_cast<uint32_t>(nextDesc.layout.pushConstants.size());
  layoutInfo.pPushConstantRanges = nextDesc.layout.pushConstants.empty()
                                       ? nullptr
                                       : nextDesc.layout.pushConstants.data();

  auto next = std::make_shared<Version>(device_);
  if (vkCreatePipelineLayout(device_.device(), &layoutInfo, nullptr,
                             &next->layout_) != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create compute pipeline layout for '{}'",
                   nextDesc.name);
  }

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.stage = shaderStage;
  pipelineInfo.layout = next->layout_;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  if (vkCreateComputePipelines(device_.device(), VK_NULL_HANDLE, 1,
                               &pipelineInfo, nullptr,
                               &next->pipeline_) != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create compute pipeline '{}'", nextDesc.name);
  }

  shader_module_ = std::move(nextShaderModule);
  desc_ = std::move(nextDesc);
  version_ = std::move(next);
  ++revision_;

  VKR_PIPE_INFO("Compute pipeline '{}' created", desc_.name);
  return true;
}

} // namespace vkr::pipe
