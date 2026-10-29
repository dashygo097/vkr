#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/logger.hh"
#include "vkr/pipeline/render_pass.hh"

namespace vkr::pipe {

GraphicsPipeline::GraphicsPipeline(const core::Device &device,
                                   const RenderPass &renderPass)
    : device_(device), render_pass_(renderPass) {}

GraphicsPipeline::~GraphicsPipeline() { destroy(); }

GraphicsPipeline::Version::~Version() {
  if (pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_.device(), pipeline_, nullptr);
  }
  if (layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_.device(), layout_, nullptr);
  }
}

void GraphicsPipeline::destroy() {
  version_.reset();
  shader_modules_.clear();
}

auto GraphicsPipeline::update(const GraphicsPipelineDesc &desc) -> bool {
  auto nextDesc = desc;

  if (!nextDesc.isValid()) {
    VKR_PIPE_ERROR("Invalid graphics pipeline descriptor");
  }

  if (render_pass_.renderPass() == VK_NULL_HANDLE) {
    VKR_PIPE_ERROR("Graphics pipeline '{}' requires a valid render pass",
                   nextDesc.name);
  }

  std::vector<std::unique_ptr<res::ShaderModule>> nextShaderModules{};
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};

  nextShaderModules.reserve(nextDesc.shaders.size());
  shaderStages.reserve(nextDesc.shaders.size());

  for (const auto &shader : nextDesc.shaders) {
    auto module = std::make_unique<res::ShaderModule>(device_);
    module->update(shader.module);

    if (!module->valid()) {
      VKR_PIPE_ERROR("Failed to create shader module for pipeline '{}'",
                     nextDesc.name);
    }

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = shader.stage;
    stage.module = module->module();
    stage.pName = shader.entryPoint.c_str();

    shaderStages.push_back(stage);
    nextShaderModules.push_back(std::move(module));
  }

  auto vertexInput = nextDesc.vertexInput.createInfo();
  auto inputAssembly = nextDesc.inputAssembly.createInfo();
  auto tessellation = nextDesc.tessellation.createInfo();
  auto viewport = nextDesc.viewport.createInfo();
  auto rasterization = nextDesc.rasterization.createInfo();
  auto multisample = nextDesc.multisample.createInfo();
  auto depthStencil = nextDesc.depthStencil.createInfo();
  auto colorBlend = nextDesc.colorBlend.createInfo();
  auto dynamicState = nextDesc.dynamicState.createInfo();

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
    VKR_PIPE_ERROR("Failed to create graphics pipeline layout for '{}'",
                   desc.name);
  }

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.flags = nextDesc.flags;
  pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInput;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pTessellationState =
      nextDesc.tessellation.enabled() ? &tessellation : nullptr;
  pipelineInfo.pViewportState = &viewport;
  pipelineInfo.pRasterizationState = &rasterization;
  pipelineInfo.pMultisampleState = &multisample;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlend;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = next->layout_;
  pipelineInfo.renderPass = render_pass_.renderPass();
  pipelineInfo.subpass = nextDesc.subpass;
  pipelineInfo.basePipelineHandle = nextDesc.basePipeline;
  pipelineInfo.basePipelineIndex = nextDesc.basePipelineIndex;

  if (vkCreateGraphicsPipelines(device_.device(), VK_NULL_HANDLE, 1,
                                &pipelineInfo, nullptr,
                                &next->pipeline_) != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create graphics pipeline '{}'", desc.name);
  }

  shader_modules_ = std::move(nextShaderModules);
  desc_ = std::move(nextDesc);
  version_ = std::move(next);
  ++revision_;

  VKR_PIPE_INFO("Graphics pipeline '{}' created", desc_.name);
  return true;
}

} // namespace vkr::pipe
