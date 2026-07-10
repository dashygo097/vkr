#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/logger.hh"

namespace vkr::pipeline {

GraphicsPipeline::GraphicsPipeline(const core::Device &device)
    : device_(&device) {}

GraphicsPipeline::~GraphicsPipeline() { destroy(); }

void GraphicsPipeline::create() {
  destroy();

  if (device_ == nullptr) {
    VKR_PIPE_ERROR("Cannot create graphics pipeline without device");
    return;
  }

  if (!desc_.isValid()) {
    VKR_PIPE_ERROR("Invalid graphics pipeline descriptor");
    return;
  }

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
  shaderStages.reserve(desc_.shaders.size());
  shader_modules_.reserve(desc_.shaders.size());

  for (const auto &shader : desc_.shaders) {
    auto module = std::make_unique<ShaderModule>(*device_);
    module->update(shader.module);

    if (!module->valid()) {
      VKR_PIPE_ERROR("Failed to create shader module for pipeline '{}'",
                     desc_.name);
      shader_modules_.clear();
      return;
    }

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = shader.stage;
    stage.module = module->module();
    stage.pName = shader.entryPoint.c_str();

    shaderStages.push_back(stage);
    shader_modules_.push_back(std::move(module));
  }

  auto vertexInput = desc_.vertexInput.createInfo();
  auto inputAssembly = desc_.inputAssembly.createInfo();
  auto viewport = desc_.viewport.createInfo();
  auto rasterization = desc_.rasterization.createInfo();
  auto multisample = desc_.multisample.createInfo();
  auto depthStencil = desc_.depthStencil.createInfo();
  auto colorBlend = desc_.colorBlend.createInfo();
  auto dynamicState = desc_.dynamicState.createInfo();

  vk_pipeline_layout_ = createPipelineLayout();
  if (vk_pipeline_layout_ == VK_NULL_HANDLE) {
    VKR_PIPE_ERROR("Failed to create graphics pipeline layout");
    shader_modules_.clear();
    return;
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
  pipelineInfo.layout = vk_pipeline_layout_;
  pipelineInfo.renderPass = desc_.renderPass;
  pipelineInfo.subpass = desc_.subpass;
  pipelineInfo.basePipelineHandle = desc_.basePipeline;
  pipelineInfo.basePipelineIndex = desc_.basePipelineIndex;

  if (vkCreateGraphicsPipelines(device_->device(), VK_NULL_HANDLE, 1,
                                &pipelineInfo, nullptr,
                                &vk_graphics_pipeline_) != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create graphics pipeline '{}'", desc_.name);

    if (vk_pipeline_layout_ != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(device_->device(), vk_pipeline_layout_, nullptr);
      vk_pipeline_layout_ = VK_NULL_HANDLE;
    }

    shader_modules_.clear();
    vk_graphics_pipeline_ = VK_NULL_HANDLE;
    return;
  }

  VKR_PIPE_INFO("Graphics pipeline '{}' created", desc_.name);
}

void GraphicsPipeline::destroy() {
  if (device_ == nullptr) {
    shader_modules_.clear();
    vk_graphics_pipeline_ = VK_NULL_HANDLE;
    vk_pipeline_layout_ = VK_NULL_HANDLE;
    return;
  }

  if (vk_graphics_pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_->device(), vk_graphics_pipeline_, nullptr);
    vk_graphics_pipeline_ = VK_NULL_HANDLE;
  }

  if (vk_pipeline_layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_->device(), vk_pipeline_layout_, nullptr);
    vk_pipeline_layout_ = VK_NULL_HANDLE;
  }

  shader_modules_.clear();
}

void GraphicsPipeline::update(const GraphicsPipelineDesc &desc) {
  desc_ = desc;
  create();
}

auto GraphicsPipeline::createPipelineLayout() const -> VkPipelineLayout {
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

  VkPipelineLayout layout{VK_NULL_HANDLE};
  if (vkCreatePipelineLayout(device_->device(), &layoutInfo, nullptr,
                             &layout) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }

  return layout;
}

} // namespace vkr::pipeline
