#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/shader_module.hh"
#include "vkr/resource/buffers/vbos.hh"
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::pipeline {

struct GraphicsShaderStageDesc {
  VkShaderStageFlagBits stage{VK_SHADER_STAGE_VERTEX_BIT};
  ShaderModuleDesc module{};
  std::string entryPoint{"main"};

  [[nodiscard]] static auto make(VkShaderStageFlagBits stage,
                                 ShaderModuleDesc module,
                                 std::string entryPoint = "main")
      -> GraphicsShaderStageDesc {
    GraphicsShaderStageDesc desc{};
    desc.stage = stage;
    desc.module = std::move(module);
    desc.entryPoint = std::move(entryPoint);
    return desc;
  }

  [[nodiscard]] static auto vertex(ShaderModuleDesc module,
                                   std::string entryPoint = "main")
      -> GraphicsShaderStageDesc {
    return make(VK_SHADER_STAGE_VERTEX_BIT, std::move(module),
                std::move(entryPoint));
  }

  [[nodiscard]] static auto fragment(ShaderModuleDesc module,
                                     std::string entryPoint = "main")
      -> GraphicsShaderStageDesc {
    return make(VK_SHADER_STAGE_FRAGMENT_BIT, std::move(module),
                std::move(entryPoint));
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return !entryPoint.empty() && module.isValid();
  }
};

struct GraphicsPipelineLayoutDesc {
  std::vector<VkDescriptorSetLayout> setLayouts{};
  std::vector<VkPushConstantRange> pushConstants{};
};

struct GraphicsInputAssemblyDesc {
  VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
  VkBool32 primitiveRestartEnable{VK_FALSE};

  [[nodiscard]] auto createInfo() const noexcept
      -> VkPipelineInputAssemblyStateCreateInfo {
    VkPipelineInputAssemblyStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.topology = topology;
    info.primitiveRestartEnable = primitiveRestartEnable;
    return info;
  }
};

struct GraphicsViewportDesc {
  uint32_t viewportCount{1};
  uint32_t scissorCount{1};

  [[nodiscard]] auto createInfo() const noexcept
      -> VkPipelineViewportStateCreateInfo {
    VkPipelineViewportStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info.viewportCount = viewportCount;
    info.scissorCount = scissorCount;
    return info;
  }
};

struct GraphicsRasterizationDesc {
  VkBool32 depthClampEnable{VK_FALSE};
  VkBool32 rasterizerDiscardEnable{VK_FALSE};
  VkPolygonMode polygonMode{VK_POLYGON_MODE_FILL};
  VkCullModeFlags cullMode{VK_CULL_MODE_BACK_BIT};
  VkFrontFace frontFace{VK_FRONT_FACE_COUNTER_CLOCKWISE};
  VkBool32 depthBiasEnable{VK_FALSE};
  float depthBiasConstantFactor{0.0f};
  float depthBiasClamp{0.0f};
  float depthBiasSlopeFactor{0.0f};
  float lineWidth{1.0f};

  [[nodiscard]] static auto noCull() -> GraphicsRasterizationDesc {
    GraphicsRasterizationDesc desc{};
    desc.cullMode = VK_CULL_MODE_NONE;
    return desc;
  }

  [[nodiscard]] auto createInfo() const noexcept
      -> VkPipelineRasterizationStateCreateInfo {
    VkPipelineRasterizationStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.depthClampEnable = depthClampEnable;
    info.rasterizerDiscardEnable = rasterizerDiscardEnable;
    info.polygonMode = polygonMode;
    info.cullMode = cullMode;
    info.frontFace = frontFace;
    info.depthBiasEnable = depthBiasEnable;
    info.depthBiasConstantFactor = depthBiasConstantFactor;
    info.depthBiasClamp = depthBiasClamp;
    info.depthBiasSlopeFactor = depthBiasSlopeFactor;
    info.lineWidth = lineWidth;
    return info;
  }
};

struct GraphicsMultisampleDesc {
  VkSampleCountFlagBits rasterizationSamples{VK_SAMPLE_COUNT_1_BIT};
  VkBool32 sampleShadingEnable{VK_FALSE};
  float minSampleShading{1.0f};
  const VkSampleMask *sampleMask{nullptr};
  VkBool32 alphaToCoverageEnable{VK_FALSE};
  VkBool32 alphaToOneEnable{VK_FALSE};

  [[nodiscard]] auto createInfo() const noexcept
      -> VkPipelineMultisampleStateCreateInfo {
    VkPipelineMultisampleStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.rasterizationSamples = rasterizationSamples;
    info.sampleShadingEnable = sampleShadingEnable;
    info.minSampleShading = minSampleShading;
    info.pSampleMask = sampleMask;
    info.alphaToCoverageEnable = alphaToCoverageEnable;
    info.alphaToOneEnable = alphaToOneEnable;
    return info;
  }
};

struct GraphicsDepthStencilDesc {
  VkBool32 depthTestEnable{VK_TRUE};
  VkBool32 depthWriteEnable{VK_TRUE};
  VkCompareOp depthCompareOp{VK_COMPARE_OP_LESS};
  VkBool32 depthBoundsTestEnable{VK_FALSE};
  VkBool32 stencilTestEnable{VK_FALSE};
  VkStencilOpState front{};
  VkStencilOpState back{};
  float minDepthBounds{0.0f};
  float maxDepthBounds{1.0f};

  [[nodiscard]] static auto disabled() -> GraphicsDepthStencilDesc {
    GraphicsDepthStencilDesc desc{};
    desc.depthTestEnable = VK_FALSE;
    desc.depthWriteEnable = VK_FALSE;
    desc.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    return desc;
  }

  [[nodiscard]] static auto readOnly() -> GraphicsDepthStencilDesc {
    GraphicsDepthStencilDesc desc{};
    desc.depthTestEnable = VK_TRUE;
    desc.depthWriteEnable = VK_FALSE;
    desc.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    return desc;
  }

  [[nodiscard]] auto createInfo() const noexcept
      -> VkPipelineDepthStencilStateCreateInfo {
    VkPipelineDepthStencilStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.depthTestEnable = depthTestEnable;
    info.depthWriteEnable = depthWriteEnable;
    info.depthCompareOp = depthCompareOp;
    info.depthBoundsTestEnable = depthBoundsTestEnable;
    info.stencilTestEnable = stencilTestEnable;
    info.front = front;
    info.back = back;
    info.minDepthBounds = minDepthBounds;
    info.maxDepthBounds = maxDepthBounds;
    return info;
  }
};

struct GraphicsColorBlendDesc {
  VkBool32 logicOpEnable{VK_FALSE};
  VkLogicOp logicOp{VK_LOGIC_OP_COPY};
  std::vector<VkPipelineColorBlendAttachmentState> attachments{};
  float blendConstants[4]{0.0f, 0.0f, 0.0f, 0.0f};

  GraphicsColorBlendDesc() : attachments{opaqueAttachment()} {}

  [[nodiscard]] static auto opaqueAttachment()
      -> VkPipelineColorBlendAttachmentState {
    VkPipelineColorBlendAttachmentState attachment{};
    attachment.blendEnable = VK_FALSE;
    attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    return attachment;
  }

  [[nodiscard]] static auto alphaBlendAttachment()
      -> VkPipelineColorBlendAttachmentState {
    VkPipelineColorBlendAttachmentState attachment{};
    attachment.blendEnable = VK_TRUE;
    attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.colorBlendOp = VK_BLEND_OP_ADD;
    attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    return attachment;
  }

  [[nodiscard]] static auto none() -> GraphicsColorBlendDesc {
    GraphicsColorBlendDesc desc{};
    desc.attachments.clear();
    return desc;
  }

  [[nodiscard]] static auto alphaBlend() -> GraphicsColorBlendDesc {
    GraphicsColorBlendDesc desc{};
    desc.attachments = {alphaBlendAttachment()};
    return desc;
  }

  [[nodiscard]] auto createInfo() const noexcept
      -> VkPipelineColorBlendStateCreateInfo {
    VkPipelineColorBlendStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    info.logicOpEnable = logicOpEnable;
    info.logicOp = logicOp;
    info.attachmentCount = static_cast<uint32_t>(attachments.size());
    info.pAttachments = attachments.empty() ? nullptr : attachments.data();
    info.blendConstants[0] = blendConstants[0];
    info.blendConstants[1] = blendConstants[1];
    info.blendConstants[2] = blendConstants[2];
    info.blendConstants[3] = blendConstants[3];
    return info;
  }
};

struct GraphicsDynamicStateDesc {
  std::vector<VkDynamicState> states{VK_DYNAMIC_STATE_VIEWPORT,
                                     VK_DYNAMIC_STATE_SCISSOR};

  [[nodiscard]] auto createInfo() const noexcept
      -> VkPipelineDynamicStateCreateInfo {
    VkPipelineDynamicStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    info.dynamicStateCount = static_cast<uint32_t>(states.size());
    info.pDynamicStates = states.empty() ? nullptr : states.data();
    return info;
  }
};

struct GraphicsPipelineDesc {
  std::string name{};
  std::vector<GraphicsShaderStageDesc> shaders{};
  resource::VertexInputDesc vertexInput{};

  GraphicsPipelineLayoutDesc layout{};
  GraphicsInputAssemblyDesc inputAssembly{};
  GraphicsViewportDesc viewport{};
  GraphicsRasterizationDesc rasterization{};
  GraphicsMultisampleDesc multisample{};
  GraphicsDepthStencilDesc depthStencil{};
  GraphicsColorBlendDesc colorBlend{};
  GraphicsDynamicStateDesc dynamicState{};

  VkRenderPass renderPass{VK_NULL_HANDLE};
  uint32_t subpass{0};

  VkPipeline basePipeline{VK_NULL_HANDLE};
  int32_t basePipelineIndex{-1};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    if (renderPass == VK_NULL_HANDLE || shaders.empty() || name.empty()) {
      return false;
    }

    for (const auto &shader : shaders) {
      if (!shader.isValid()) {
        return false;
      }
    }

    return true;
  }
};

class GraphicsPipeline {
public:
  explicit GraphicsPipeline(const core::Device &device);
  ~GraphicsPipeline();

  GraphicsPipeline(const GraphicsPipeline &) = delete;
  auto operator=(const GraphicsPipeline &) -> GraphicsPipeline & = delete;

  void create();
  void destroy();
  void update(const GraphicsPipelineDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const GraphicsPipelineDesc & {
    return desc_;
  }

  [[nodiscard]] auto shaders() const noexcept
      -> const std::vector<GraphicsShaderStageDesc> & {
    return desc_.shaders;
  }

  [[nodiscard]] auto shaderModules() const noexcept
      -> const std::vector<std::unique_ptr<ShaderModule>> & {
    return shader_modules_;
  }

  [[nodiscard]] auto layout() const noexcept -> VkPipelineLayout {
    return vk_pipeline_layout_;
  }

  [[nodiscard]] auto pipeline() const noexcept -> VkPipeline {
    return vk_graphics_pipeline_;
  }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return vk_graphics_pipeline_ != VK_NULL_HANDLE;
  }

private:
  // dependencies
  const core::Device *device_{nullptr};

  // components
  GraphicsPipelineDesc desc_{};
  std::vector<std::unique_ptr<ShaderModule>> shader_modules_{};
  VkPipelineLayout vk_pipeline_layout_{VK_NULL_HANDLE};
  VkPipeline vk_graphics_pipeline_{VK_NULL_HANDLE};

  // helpers
  [[nodiscard]] auto createPipelineLayout() const -> VkPipelineLayout;
};

} // namespace vkr::pipeline
