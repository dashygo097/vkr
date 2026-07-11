#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/logger.hh"

namespace vkr::pipeline {

GraphicsPipeline::GraphicsPipeline(const core::Device &device)
    : device_(&device) {}

GraphicsPipeline::~GraphicsPipeline() { destroy(); }

void GraphicsPipeline::destroy() {
  if (device_ == nullptr) {
    shader_modules_.clear();
    retired_pipelines_.clear();
    vk_graphics_pipeline_ = VK_NULL_HANDLE;
    vk_pipeline_layout_ = VK_NULL_HANDLE;
    return;
  }

  if (vk_graphics_pipeline_ != VK_NULL_HANDLE ||
      vk_pipeline_layout_ != VK_NULL_HANDLE || !retired_pipelines_.empty()) {
    vkDeviceWaitIdle(device_->device());
  }

  if (vk_graphics_pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_->device(), vk_graphics_pipeline_, nullptr);
    vk_graphics_pipeline_ = VK_NULL_HANDLE;
  }

  if (vk_pipeline_layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_->device(), vk_pipeline_layout_, nullptr);
    vk_pipeline_layout_ = VK_NULL_HANDLE;
  }

  for (const auto &retired : retired_pipelines_) {
    if (retired.pipeline != VK_NULL_HANDLE) {
      vkDestroyPipeline(device_->device(), retired.pipeline, nullptr);
    }

    if (retired.layout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(device_->device(), retired.layout, nullptr);
    }
  }

  retired_pipelines_.clear();
  shader_modules_.clear();
}

auto GraphicsPipeline::update(const GraphicsPipelineDesc &desc) -> bool {
  const GraphicsPipelineDesc oldDesc = desc_;
  desc_ = desc;

  if (device_ == nullptr) {
    desc_ = oldDesc;
    VKR_PIPE_ERROR("Cannot create graphics pipeline without device");
    return false;
  }

  if (!desc_.isValid()) {
    desc_ = oldDesc;
    VKR_PIPE_ERROR("Invalid graphics pipeline descriptor");
    return false;
  }

  std::vector<std::unique_ptr<ShaderModule>> nextShaderModules{};
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};

  nextShaderModules.reserve(desc_.shaders.size());
  shaderStages.reserve(desc_.shaders.size());

  for (const auto &shader : desc_.shaders) {
    auto module = std::make_unique<ShaderModule>(*device_);
    module->update(shader.module);

    if (!module->valid()) {
      desc_ = oldDesc;
      VKR_PIPE_ERROR("Failed to create shader module for pipeline '{}'",
                     desc_.name);
      return false;
    }

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = shader.stage;
    stage.module = module->module();
    stage.pName = shader.entryPoint.c_str();

    shaderStages.push_back(stage);
    nextShaderModules.push_back(std::move(module));
  }

  auto vertexInput = desc_.vertexInput.createInfo();
  auto inputAssembly = desc_.inputAssembly.createInfo();
  auto viewport = desc_.viewport.createInfo();
  auto rasterization = desc_.rasterization.createInfo();
  auto multisample = desc_.multisample.createInfo();
  auto depthStencil = desc_.depthStencil.createInfo();
  auto colorBlend = desc_.colorBlend.createInfo();
  auto dynamicState = desc_.dynamicState.createInfo();

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

  VkPipelineLayout nextLayout = VK_NULL_HANDLE;
  if (vkCreatePipelineLayout(device_->device(), &layoutInfo, nullptr,
                             &nextLayout) != VK_SUCCESS) {
    desc_ = oldDesc;
    VKR_PIPE_ERROR("Failed to create graphics pipeline layout for '{}'",
                   desc.name);
    return false;
  }

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInput;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewport;
  pipelineInfo.pRasterizationState = &rasterization;
  pipelineInfo.pMultisampleState = &multisample;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlend;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = nextLayout;
  pipelineInfo.renderPass = desc_.renderPass;
  pipelineInfo.subpass = desc_.subpass;
  pipelineInfo.basePipelineHandle = desc_.basePipeline;
  pipelineInfo.basePipelineIndex = desc_.basePipelineIndex;

  VkPipeline nextPipeline = VK_NULL_HANDLE;
  if (vkCreateGraphicsPipelines(device_->device(), VK_NULL_HANDLE, 1,
                                &pipelineInfo, nullptr,
                                &nextPipeline) != VK_SUCCESS) {
    vkDestroyPipelineLayout(device_->device(), nextLayout, nullptr);
    desc_ = oldDesc;
    VKR_PIPE_ERROR("Failed to create graphics pipeline '{}'", desc.name);
    return false;
  }

  if (vk_graphics_pipeline_ != VK_NULL_HANDLE ||
      vk_pipeline_layout_ != VK_NULL_HANDLE) {
    RetiredPipeline retired{};
    retired.pipeline = vk_graphics_pipeline_;
    retired.layout = vk_pipeline_layout_;
    retired_pipelines_.push_back(retired);
  }

  shader_modules_ = std::move(nextShaderModules);
  vk_pipeline_layout_ = nextLayout;
  vk_graphics_pipeline_ = nextPipeline;

  VKR_PIPE_INFO("Graphics pipeline '{}' created", desc_.name);
  return true;
}

} // namespace vkr::pipeline
