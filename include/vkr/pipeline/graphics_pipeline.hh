#pragma once

#include "vkr/core/device.hh"
#include "vkr/resource/shader/module.hh"
#include "vkr/scene/geometry/vbos.hh"
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace vkr::pipeline {

class RenderPass;

struct GraphicsShaderStageDesc {
  VkShaderStageFlagBits stage{VK_SHADER_STAGE_VERTEX_BIT};
  resource::ShaderModuleDesc module{};
  std::string entryPoint{"main"};

  auto shaderStage(VkShaderStageFlagBits shaderStage) noexcept
      -> GraphicsShaderStageDesc & {
    stage = shaderStage;
    return *this;
  }

  auto shaderModule(resource::ShaderModuleDesc shaderModule)
      -> GraphicsShaderStageDesc & {
    module = std::move(shaderModule);
    return *this;
  }

  auto entry(std::string name) -> GraphicsShaderStageDesc & {
    entryPoint = std::move(name);
    return *this;
  }

  [[nodiscard]] static auto make(VkShaderStageFlagBits stage,
                                 resource::ShaderModuleDesc module,
                                 std::string entryPoint = "main")
      -> GraphicsShaderStageDesc {
    GraphicsShaderStageDesc desc{};
    desc.stage = stage;
    desc.module = std::move(module);
    desc.entryPoint = std::move(entryPoint);
    return desc;
  }

  [[nodiscard]] static auto vertex(resource::ShaderModuleDesc module,
                                   std::string entryPoint = "main")
      -> GraphicsShaderStageDesc {
    return make(VK_SHADER_STAGE_VERTEX_BIT, std::move(module),
                std::move(entryPoint));
  }

  [[nodiscard]] static auto fragment(resource::ShaderModuleDesc module,
                                     std::string entryPoint = "main")
      -> GraphicsShaderStageDesc {
    return make(VK_SHADER_STAGE_FRAGMENT_BIT, std::move(module),
                std::move(entryPoint));
  }

  [[nodiscard]] static auto geometry(resource::ShaderModuleDesc module,
                                     std::string entryPoint = "main")
      -> GraphicsShaderStageDesc {
    return make(VK_SHADER_STAGE_GEOMETRY_BIT, std::move(module),
                std::move(entryPoint));
  }

  [[nodiscard]] static auto tessControl(resource::ShaderModuleDesc module,
                                        std::string entryPoint = "main")
      -> GraphicsShaderStageDesc {
    return make(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, std::move(module),
                std::move(entryPoint));
  }

  [[nodiscard]] static auto tessEvaluation(resource::ShaderModuleDesc module,
                                           std::string entryPoint = "main")
      -> GraphicsShaderStageDesc {
    return make(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, std::move(module),
                std::move(entryPoint));
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return !entryPoint.empty() && module.isValid();
  }
};

struct GraphicsPipelineLayoutDesc {
  std::vector<VkDescriptorSetLayout> setLayouts{};
  std::vector<VkPushConstantRange> pushConstants{};

  auto setLayout(VkDescriptorSetLayout layout) -> GraphicsPipelineLayoutDesc & {
    setLayouts.push_back(layout);
    return *this;
  }

  auto setLayout(uint32_t index, VkDescriptorSetLayout layout)
      -> GraphicsPipelineLayoutDesc & {
    if (setLayouts.size() <= index) {
      setLayouts.resize(static_cast<size_t>(index) + 1U, VK_NULL_HANDLE);
    }
    setLayouts[index] = layout;
    return *this;
  }

  auto descriptorSetLayouts(std::vector<VkDescriptorSetLayout> layouts)
      -> GraphicsPipelineLayoutDesc & {
    setLayouts = std::move(layouts);
    return *this;
  }

  auto clearSetLayouts() noexcept -> GraphicsPipelineLayoutDesc & {
    setLayouts.clear();
    return *this;
  }

  auto pushConstant(VkPushConstantRange range) -> GraphicsPipelineLayoutDesc & {
    pushConstants.push_back(range);
    return *this;
  }

  auto pushConstant(VkShaderStageFlags stageFlags, uint32_t offset,
                    uint32_t size) -> GraphicsPipelineLayoutDesc & {
    pushConstants.push_back({stageFlags, offset, size});
    return *this;
  }

  auto pushConstantRanges(std::vector<VkPushConstantRange> ranges)
      -> GraphicsPipelineLayoutDesc & {
    pushConstants = std::move(ranges);
    return *this;
  }

  auto clearPushConstants() noexcept -> GraphicsPipelineLayoutDesc & {
    pushConstants.clear();
    return *this;
  }

  [[nodiscard]] static auto empty() -> GraphicsPipelineLayoutDesc { return {}; }
};

struct GraphicsInputAssemblyDesc {
  VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
  VkBool32 primitiveRestartEnable{VK_FALSE};

  auto primitiveTopology(VkPrimitiveTopology value) noexcept
      -> GraphicsInputAssemblyDesc & {
    topology = value;
    return *this;
  }

  auto primitiveRestart(bool enabled = true) noexcept
      -> GraphicsInputAssemblyDesc & {
    primitiveRestartEnable = enabled ? VK_TRUE : VK_FALSE;
    return *this;
  }

  auto points() noexcept -> GraphicsInputAssemblyDesc & {
    topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    primitiveRestartEnable = VK_FALSE;
    return *this;
  }

  auto lines(bool strip = false, bool restart = false) noexcept
      -> GraphicsInputAssemblyDesc & {
    topology = strip ? VK_PRIMITIVE_TOPOLOGY_LINE_STRIP
                     : VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    primitiveRestartEnable = restart ? VK_TRUE : VK_FALSE;
    return *this;
  }

  auto triangles(bool strip = false, bool restart = false) noexcept
      -> GraphicsInputAssemblyDesc & {
    topology = strip ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
                     : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    primitiveRestartEnable = restart ? VK_TRUE : VK_FALSE;
    return *this;
  }

  auto patches() noexcept -> GraphicsInputAssemblyDesc & {
    topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    primitiveRestartEnable = VK_FALSE;
    return *this;
  }

  [[nodiscard]] static auto pointList() -> GraphicsInputAssemblyDesc {
    GraphicsInputAssemblyDesc desc{};
    return desc.points();
  }

  [[nodiscard]] static auto lineList() -> GraphicsInputAssemblyDesc {
    GraphicsInputAssemblyDesc desc{};
    return desc.lines();
  }

  [[nodiscard]] static auto lineStrip(bool restart = false)
      -> GraphicsInputAssemblyDesc {
    GraphicsInputAssemblyDesc desc{};
    return desc.lines(true, restart);
  }

  [[nodiscard]] static auto triangleList() -> GraphicsInputAssemblyDesc {
    GraphicsInputAssemblyDesc desc{};
    return desc.triangles();
  }

  [[nodiscard]] static auto triangleStrip(bool restart = false)
      -> GraphicsInputAssemblyDesc {
    GraphicsInputAssemblyDesc desc{};
    return desc.triangles(true, restart);
  }

  [[nodiscard]] static auto patchList() -> GraphicsInputAssemblyDesc {
    GraphicsInputAssemblyDesc desc{};
    return desc.patches();
  }

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

  auto viewports(uint32_t count) noexcept -> GraphicsViewportDesc & {
    viewportCount = count;
    return *this;
  }

  auto scissors(uint32_t count) noexcept -> GraphicsViewportDesc & {
    scissorCount = count;
    return *this;
  }

  auto counts(uint32_t viewports, uint32_t scissors) noexcept
      -> GraphicsViewportDesc & {
    viewportCount = viewports;
    scissorCount = scissors;
    return *this;
  }

  [[nodiscard]] static auto single() -> GraphicsViewportDesc { return {}; }

  [[nodiscard]] static auto multi(uint32_t count) -> GraphicsViewportDesc {
    GraphicsViewportDesc desc{};
    return desc.counts(count, count);
  }

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

  auto depthClamp(bool enabled = true) noexcept -> GraphicsRasterizationDesc & {
    depthClampEnable = enabled ? VK_TRUE : VK_FALSE;
    return *this;
  }

  auto rasterizerDiscard(bool enabled = true) noexcept
      -> GraphicsRasterizationDesc & {
    rasterizerDiscardEnable = enabled ? VK_TRUE : VK_FALSE;
    return *this;
  }

  auto polygon(VkPolygonMode mode) noexcept -> GraphicsRasterizationDesc & {
    polygonMode = mode;
    return *this;
  }

  auto fill() noexcept -> GraphicsRasterizationDesc & {
    polygonMode = VK_POLYGON_MODE_FILL;
    return *this;
  }

  auto wireframe() noexcept -> GraphicsRasterizationDesc & {
    polygonMode = VK_POLYGON_MODE_LINE;
    return *this;
  }

  auto points() noexcept -> GraphicsRasterizationDesc & {
    polygonMode = VK_POLYGON_MODE_POINT;
    return *this;
  }

  auto culling(VkCullModeFlags mode) noexcept -> GraphicsRasterizationDesc & {
    cullMode = mode;
    return *this;
  }

  auto disableCull() noexcept -> GraphicsRasterizationDesc & {
    cullMode = VK_CULL_MODE_NONE;
    return *this;
  }

  auto backFaceCull() noexcept -> GraphicsRasterizationDesc & {
    cullMode = VK_CULL_MODE_BACK_BIT;
    return *this;
  }

  auto frontFaceCull() noexcept -> GraphicsRasterizationDesc & {
    cullMode = VK_CULL_MODE_FRONT_BIT;
    return *this;
  }

  auto cullBoth() noexcept -> GraphicsRasterizationDesc & {
    cullMode = VK_CULL_MODE_FRONT_AND_BACK;
    return *this;
  }

  auto frontFacing(VkFrontFace face) noexcept -> GraphicsRasterizationDesc & {
    frontFace = face;
    return *this;
  }

  auto clockwise() noexcept -> GraphicsRasterizationDesc & {
    frontFace = VK_FRONT_FACE_CLOCKWISE;
    return *this;
  }

  auto counterClockwise() noexcept -> GraphicsRasterizationDesc & {
    frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    return *this;
  }

  auto depthBias(bool enabled = true) noexcept -> GraphicsRasterizationDesc & {
    depthBiasEnable = enabled ? VK_TRUE : VK_FALSE;
    return *this;
  }

  auto depthBias(float constantFactor, float slopeFactor,
                 float clamp = 0.0f) noexcept -> GraphicsRasterizationDesc & {
    depthBiasEnable = VK_TRUE;
    depthBiasConstantFactor = constantFactor;
    depthBiasSlopeFactor = slopeFactor;
    depthBiasClamp = clamp;
    return *this;
  }

  auto line(float width = 1.0f) noexcept -> GraphicsRasterizationDesc & {
    lineWidth = width;
    return *this;
  }

  [[nodiscard]] static auto noCull() -> GraphicsRasterizationDesc {
    GraphicsRasterizationDesc desc{};
    return desc.disableCull();
  }

  [[nodiscard]] static auto cull(VkCullModeFlags mode)
      -> GraphicsRasterizationDesc {
    GraphicsRasterizationDesc desc{};
    return desc.culling(mode);
  }

  [[nodiscard]] static auto cullBack() -> GraphicsRasterizationDesc {
    return cull(VK_CULL_MODE_BACK_BIT);
  }

  [[nodiscard]] static auto cullFront() -> GraphicsRasterizationDesc {
    return cull(VK_CULL_MODE_FRONT_BIT);
  }

  [[nodiscard]] static auto wireframeNoCull(float width = 1.0f)
      -> GraphicsRasterizationDesc {
    GraphicsRasterizationDesc desc{};
    return desc.wireframe().disableCull().line(width);
  }

  [[nodiscard]] static auto
  shadow(float constantFactor = 1.25f, float slopeFactor = 1.75f,
         VkCullModeFlags shadowCullMode = VK_CULL_MODE_BACK_BIT,
         float clamp = 0.0f) -> GraphicsRasterizationDesc {
    GraphicsRasterizationDesc desc{};
    return desc.culling(shadowCullMode)
        .depthBias(constantFactor, slopeFactor, clamp);
  }

  [[nodiscard]] static auto discard() -> GraphicsRasterizationDesc {
    GraphicsRasterizationDesc desc{};
    return desc.rasterizerDiscard();
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
  std::vector<VkSampleMask> sampleMasks{};
  VkBool32 alphaToCoverageEnable{VK_FALSE};
  VkBool32 alphaToOneEnable{VK_FALSE};

  auto samples(VkSampleCountFlagBits count) noexcept
      -> GraphicsMultisampleDesc & {
    rasterizationSamples = count;
    return *this;
  }

  auto sampleShading(bool enabled = true, float minimum = 1.0f) noexcept
      -> GraphicsMultisampleDesc & {
    sampleShadingEnable = enabled ? VK_TRUE : VK_FALSE;
    minSampleShading = minimum;
    return *this;
  }

  auto minSample(float value) noexcept -> GraphicsMultisampleDesc & {
    minSampleShading = value;
    return *this;
  }

  auto mask(const VkSampleMask *mask) noexcept -> GraphicsMultisampleDesc & {
    sampleMask = mask;
    sampleMasks.clear();
    return *this;
  }

  auto masks(std::vector<VkSampleMask> masks) -> GraphicsMultisampleDesc & {
    sampleMasks = std::move(masks);
    sampleMask = nullptr;
    return *this;
  }

  auto alphaToCoverage(bool enabled = true) noexcept
      -> GraphicsMultisampleDesc & {
    alphaToCoverageEnable = enabled ? VK_TRUE : VK_FALSE;
    return *this;
  }

  auto alphaToOne(bool enabled = true) noexcept -> GraphicsMultisampleDesc & {
    alphaToOneEnable = enabled ? VK_TRUE : VK_FALSE;
    return *this;
  }

  [[nodiscard]] static auto none() -> GraphicsMultisampleDesc { return {}; }

  [[nodiscard]] static auto msaa(VkSampleCountFlagBits count)
      -> GraphicsMultisampleDesc {
    GraphicsMultisampleDesc desc{};
    return desc.samples(count);
  }

  [[nodiscard]] static auto alphaCoverage(VkSampleCountFlagBits count)
      -> GraphicsMultisampleDesc {
    GraphicsMultisampleDesc desc{};
    return desc.samples(count).alphaToCoverage();
  }

  [[nodiscard]] auto createInfo() const noexcept
      -> VkPipelineMultisampleStateCreateInfo {
    VkPipelineMultisampleStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.rasterizationSamples = rasterizationSamples;
    info.sampleShadingEnable = sampleShadingEnable;
    info.minSampleShading = minSampleShading;
    info.pSampleMask = sampleMasks.empty() ? sampleMask : sampleMasks.data();
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

  auto depthTest(bool enabled = true) noexcept -> GraphicsDepthStencilDesc & {
    depthTestEnable = enabled ? VK_TRUE : VK_FALSE;
    return *this;
  }

  auto depthWrite(bool enabled = true) noexcept -> GraphicsDepthStencilDesc & {
    depthWriteEnable = enabled ? VK_TRUE : VK_FALSE;
    return *this;
  }

  auto compare(VkCompareOp op) noexcept -> GraphicsDepthStencilDesc & {
    depthCompareOp = op;
    return *this;
  }

  auto depth(VkBool32 testEnable = VK_TRUE, VkBool32 writeEnable = VK_TRUE,
             VkCompareOp compareOp = VK_COMPARE_OP_LESS) noexcept
      -> GraphicsDepthStencilDesc & {
    depthTestEnable = testEnable;
    depthWriteEnable = writeEnable;
    depthCompareOp = compareOp;
    return *this;
  }

  auto disableDepth() noexcept -> GraphicsDepthStencilDesc & {
    depthTestEnable = VK_FALSE;
    depthWriteEnable = VK_FALSE;
    depthCompareOp = VK_COMPARE_OP_ALWAYS;
    return *this;
  }

  auto
  readOnlyDepth(VkCompareOp compareOp = VK_COMPARE_OP_LESS_OR_EQUAL) noexcept
      -> GraphicsDepthStencilDesc & {
    depthTestEnable = VK_TRUE;
    depthWriteEnable = VK_FALSE;
    depthCompareOp = compareOp;
    return *this;
  }

  auto reverseZ(bool writeEnable = true) noexcept
      -> GraphicsDepthStencilDesc & {
    depthTestEnable = VK_TRUE;
    depthWriteEnable = writeEnable ? VK_TRUE : VK_FALSE;
    depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    return *this;
  }

  auto depthBounds(float minDepth, float maxDepth) noexcept
      -> GraphicsDepthStencilDesc & {
    depthBoundsTestEnable = VK_TRUE;
    minDepthBounds = minDepth;
    maxDepthBounds = maxDepth;
    return *this;
  }

  auto disableDepthBounds() noexcept -> GraphicsDepthStencilDesc & {
    depthBoundsTestEnable = VK_FALSE;
    return *this;
  }

  auto stencil(bool enabled = true) noexcept -> GraphicsDepthStencilDesc & {
    stencilTestEnable = enabled ? VK_TRUE : VK_FALSE;
    return *this;
  }

  auto stencilFront(VkStencilOpState state) noexcept
      -> GraphicsDepthStencilDesc & {
    front = state;
    stencilTestEnable = VK_TRUE;
    return *this;
  }

  auto stencilBack(VkStencilOpState state) noexcept
      -> GraphicsDepthStencilDesc & {
    back = state;
    stencilTestEnable = VK_TRUE;
    return *this;
  }

  auto stencilBoth(VkStencilOpState state) noexcept
      -> GraphicsDepthStencilDesc & {
    front = state;
    back = state;
    stencilTestEnable = VK_TRUE;
    return *this;
  }

  [[nodiscard]] static auto disabled() -> GraphicsDepthStencilDesc {
    GraphicsDepthStencilDesc desc{};
    return desc.disableDepth();
  }

  [[nodiscard]] static auto readOnly() -> GraphicsDepthStencilDesc {
    GraphicsDepthStencilDesc desc{};
    return desc.readOnlyDepth();
  }

  [[nodiscard]] static auto write(VkCompareOp compareOp = VK_COMPARE_OP_LESS)
      -> GraphicsDepthStencilDesc {
    GraphicsDepthStencilDesc desc{};
    return desc.depth(VK_TRUE, VK_TRUE, compareOp);
  }

  [[nodiscard]] static auto reverseZPreset(bool writeEnable = true)
      -> GraphicsDepthStencilDesc {
    GraphicsDepthStencilDesc desc{};
    return desc.reverseZ(writeEnable);
  }

  [[nodiscard]] static auto
  shadowMap(VkCompareOp compareOp = VK_COMPARE_OP_LESS)
      -> GraphicsDepthStencilDesc {
    GraphicsDepthStencilDesc desc{};
    return desc.depth(VK_TRUE, VK_TRUE, compareOp).stencil(false);
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
  std::array<float, 4> blendConstants{0.0f, 0.0f, 0.0f, 0.0f};

  GraphicsColorBlendDesc() : attachments{opaqueAttachment()} {}

  auto logic(bool enabled = true, VkLogicOp op = VK_LOGIC_OP_COPY) noexcept
      -> GraphicsColorBlendDesc & {
    logicOpEnable = enabled ? VK_TRUE : VK_FALSE;
    logicOp = op;
    return *this;
  }

  auto disableLogic() noexcept -> GraphicsColorBlendDesc & {
    logicOpEnable = VK_FALSE;
    return *this;
  }

  auto clearAttachments() noexcept -> GraphicsColorBlendDesc & {
    attachments.clear();
    return *this;
  }

  auto attachment(VkPipelineColorBlendAttachmentState attachmentState)
      -> GraphicsColorBlendDesc & {
    attachments.push_back(attachmentState);
    return *this;
  }

  auto attachment(uint32_t index,
                  VkPipelineColorBlendAttachmentState attachmentState)
      -> GraphicsColorBlendDesc & {
    if (attachments.size() <= index) {
      attachments.resize(static_cast<size_t>(index) + 1U, opaqueAttachment());
    }
    attachments[index] = attachmentState;
    return *this;
  }

  auto attachmentCount(uint32_t count) -> GraphicsColorBlendDesc & {
    return attachmentCount(count, opaqueAttachment());
  }

  auto attachmentCount(uint32_t count,
                       VkPipelineColorBlendAttachmentState attachmentState)
      -> GraphicsColorBlendDesc & {
    attachments.assign(count, attachmentState);
    return *this;
  }

  auto opaque(uint32_t count = 1) -> GraphicsColorBlendDesc & {
    return attachmentCount(count, opaqueAttachment());
  }

  auto alpha(uint32_t count = 1) -> GraphicsColorBlendDesc & {
    return attachmentCount(count, alphaBlendAttachment());
  }

  auto premultipliedAlpha(uint32_t count = 1) -> GraphicsColorBlendDesc & {
    return attachmentCount(count, premultipliedAlphaAttachment());
  }

  auto additive(uint32_t count = 1) -> GraphicsColorBlendDesc & {
    return attachmentCount(count, additiveAttachment());
  }

  auto colorWriteMask(VkColorComponentFlags mask, uint32_t index = 0)
      -> GraphicsColorBlendDesc & {
    if (attachments.size() <= index) {
      attachments.resize(static_cast<size_t>(index) + 1U, opaqueAttachment());
    }
    attachments[index].colorWriteMask = mask;
    return *this;
  }

  auto blendConstant(float r, float g, float b, float a) noexcept
      -> GraphicsColorBlendDesc & {
    blendConstants = {r, g, b, a};
    return *this;
  }

  [[nodiscard]] static auto opaqueAttachment()
      -> VkPipelineColorBlendAttachmentState {
    VkPipelineColorBlendAttachmentState attachment{};
    attachment.blendEnable = VK_FALSE;
    attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    return attachment;
  }

  [[nodiscard]] static auto
  blendAttachment(VkBlendFactor srcColor, VkBlendFactor dstColor,
                  VkBlendOp colorOp, VkBlendFactor srcAlpha,
                  VkBlendFactor dstAlpha, VkBlendOp alphaOp,
                  VkColorComponentFlags colorMask = VK_COLOR_COMPONENT_R_BIT |
                                                    VK_COLOR_COMPONENT_G_BIT |
                                                    VK_COLOR_COMPONENT_B_BIT |
                                                    VK_COLOR_COMPONENT_A_BIT)
      -> VkPipelineColorBlendAttachmentState {
    VkPipelineColorBlendAttachmentState attachment{};
    attachment.blendEnable = VK_TRUE;
    attachment.srcColorBlendFactor = srcColor;
    attachment.dstColorBlendFactor = dstColor;
    attachment.colorBlendOp = colorOp;
    attachment.srcAlphaBlendFactor = srcAlpha;
    attachment.dstAlphaBlendFactor = dstAlpha;
    attachment.alphaBlendOp = alphaOp;
    attachment.colorWriteMask = colorMask;
    return attachment;
  }

  [[nodiscard]] static auto alphaBlendAttachment()
      -> VkPipelineColorBlendAttachmentState {
    return blendAttachment(VK_BLEND_FACTOR_SRC_ALPHA,
                           VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                           VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                           VK_BLEND_OP_ADD);
  }

  [[nodiscard]] static auto premultipliedAlphaAttachment()
      -> VkPipelineColorBlendAttachmentState {
    return blendAttachment(VK_BLEND_FACTOR_ONE,
                           VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                           VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                           VK_BLEND_OP_ADD);
  }

  [[nodiscard]] static auto additiveAttachment()
      -> VkPipelineColorBlendAttachmentState {
    return blendAttachment(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE,
                           VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE,
                           VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD);
  }

  [[nodiscard]] static auto colorWriteDisabledAttachment()
      -> VkPipelineColorBlendAttachmentState {
    auto attachment = opaqueAttachment();
    attachment.colorWriteMask = 0;
    return attachment;
  }

  [[nodiscard]] static auto none() -> GraphicsColorBlendDesc {
    GraphicsColorBlendDesc desc{};
    desc.attachments.clear();
    return desc;
  }

  [[nodiscard]] static auto alphaBlend() -> GraphicsColorBlendDesc {
    GraphicsColorBlendDesc desc{};
    return desc.alpha();
  }

  [[nodiscard]] static auto premultipliedAlphaBlend()
      -> GraphicsColorBlendDesc {
    GraphicsColorBlendDesc desc{};
    return desc.premultipliedAlpha();
  }

  [[nodiscard]] static auto additiveBlend() -> GraphicsColorBlendDesc {
    GraphicsColorBlendDesc desc{};
    return desc.additive();
  }

  [[nodiscard]] static auto colorWriteDisabled() -> GraphicsColorBlendDesc {
    GraphicsColorBlendDesc desc{};
    return desc.attachmentCount(1, colorWriteDisabledAttachment());
  }

  [[nodiscard]] static auto depthOnly() -> GraphicsColorBlendDesc {
    return none();
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

  auto state(VkDynamicState value) -> GraphicsDynamicStateDesc & {
    states.push_back(value);
    return *this;
  }

  auto statesList(std::vector<VkDynamicState> values)
      -> GraphicsDynamicStateDesc & {
    states = std::move(values);
    return *this;
  }

  auto clear() noexcept -> GraphicsDynamicStateDesc & {
    states.clear();
    return *this;
  }

  auto viewport(bool enabled = true) -> GraphicsDynamicStateDesc & {
    return set(VK_DYNAMIC_STATE_VIEWPORT, enabled);
  }

  auto scissor(bool enabled = true) -> GraphicsDynamicStateDesc & {
    return set(VK_DYNAMIC_STATE_SCISSOR, enabled);
  }

  auto lineWidth(bool enabled = true) -> GraphicsDynamicStateDesc & {
    return set(VK_DYNAMIC_STATE_LINE_WIDTH, enabled);
  }

  auto depthBias(bool enabled = true) -> GraphicsDynamicStateDesc & {
    return set(VK_DYNAMIC_STATE_DEPTH_BIAS, enabled);
  }

  auto blendConstants(bool enabled = true) -> GraphicsDynamicStateDesc & {
    return set(VK_DYNAMIC_STATE_BLEND_CONSTANTS, enabled);
  }

  auto depthBounds(bool enabled = true) -> GraphicsDynamicStateDesc & {
    return set(VK_DYNAMIC_STATE_DEPTH_BOUNDS, enabled);
  }

  auto stencilCompareMask(bool enabled = true) -> GraphicsDynamicStateDesc & {
    return set(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK, enabled);
  }

  auto stencilWriteMask(bool enabled = true) -> GraphicsDynamicStateDesc & {
    return set(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK, enabled);
  }

  auto stencilReference(bool enabled = true) -> GraphicsDynamicStateDesc & {
    return set(VK_DYNAMIC_STATE_STENCIL_REFERENCE, enabled);
  }

  [[nodiscard]] static auto none() -> GraphicsDynamicStateDesc {
    GraphicsDynamicStateDesc desc{};
    return desc.clear();
  }

  [[nodiscard]] static auto viewportScissor() -> GraphicsDynamicStateDesc {
    return {};
  }

  [[nodiscard]] static auto allCore() -> GraphicsDynamicStateDesc {
    GraphicsDynamicStateDesc desc{};
    return desc.statesList(
        {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
         VK_DYNAMIC_STATE_LINE_WIDTH, VK_DYNAMIC_STATE_DEPTH_BIAS,
         VK_DYNAMIC_STATE_BLEND_CONSTANTS, VK_DYNAMIC_STATE_DEPTH_BOUNDS,
         VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
         VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
         VK_DYNAMIC_STATE_STENCIL_REFERENCE});
  }

  [[nodiscard]] auto createInfo() const noexcept
      -> VkPipelineDynamicStateCreateInfo {
    VkPipelineDynamicStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    info.dynamicStateCount = static_cast<uint32_t>(states.size());
    info.pDynamicStates = states.empty() ? nullptr : states.data();
    return info;
  }

private:
  auto set(VkDynamicState value, bool enabled) -> GraphicsDynamicStateDesc & {
    const auto found = std::find(states.begin(), states.end(), value);
    if (enabled) {
      if (found == states.end()) {
        states.push_back(value);
      }
      return *this;
    }

    if (found != states.end()) {
      states.erase(found);
    }
    return *this;
  }
};

struct GraphicsTessellationDesc {
  uint32_t patchControlPoints{0};

  auto controlPoints(uint32_t count) noexcept -> GraphicsTessellationDesc & {
    patchControlPoints = count;
    return *this;
  }

  auto disable() noexcept -> GraphicsTessellationDesc & {
    patchControlPoints = 0;
    return *this;
  }

  [[nodiscard]] auto enabled() const noexcept -> bool {
    return patchControlPoints > 0;
  }

  [[nodiscard]] static auto disabled() -> GraphicsTessellationDesc {
    return {};
  }

  [[nodiscard]] static auto patches(uint32_t controlPoints)
      -> GraphicsTessellationDesc {
    GraphicsTessellationDesc desc{};
    return desc.controlPoints(controlPoints);
  }

  [[nodiscard]] auto createInfo() const noexcept
      -> VkPipelineTessellationStateCreateInfo {
    VkPipelineTessellationStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    info.patchControlPoints = patchControlPoints;
    return info;
  }
};

struct GraphicsPipelineDesc {
  std::string name{};
  VkPipelineCreateFlags flags{0};
  std::vector<GraphicsShaderStageDesc> shaders{};
  scene::VertexInputDesc vertexInput{};

  GraphicsPipelineLayoutDesc layout{};
  GraphicsInputAssemblyDesc inputAssembly{};
  GraphicsTessellationDesc tessellation{};
  GraphicsViewportDesc viewport{};
  GraphicsRasterizationDesc rasterization{};
  GraphicsMultisampleDesc multisample{};
  GraphicsDepthStencilDesc depthStencil{};
  GraphicsColorBlendDesc colorBlend{};
  GraphicsDynamicStateDesc dynamicState{};
  uint32_t subpass{0};
  VkPipeline basePipeline{VK_NULL_HANDLE};
  int32_t basePipelineIndex{-1};

  auto setName(std::string pipelineName) -> GraphicsPipelineDesc & {
    name = std::move(pipelineName);
    return *this;
  }

  auto pipelineFlags(VkPipelineCreateFlags pipelineFlags) noexcept
      -> GraphicsPipelineDesc & {
    flags = pipelineFlags;
    return *this;
  }

  auto addFlag(VkPipelineCreateFlags pipelineFlag) noexcept
      -> GraphicsPipelineDesc & {
    flags |= pipelineFlag;
    return *this;
  }

  auto vertexInputDesc(scene::VertexInputDesc desc) -> GraphicsPipelineDesc & {
    vertexInput = std::move(desc);
    return *this;
  }

  auto layoutDesc(GraphicsPipelineLayoutDesc desc) -> GraphicsPipelineDesc & {
    layout = std::move(desc);
    return *this;
  }

  auto descriptorSetLayout(VkDescriptorSetLayout setLayout)
      -> GraphicsPipelineDesc & {
    layout.setLayout(setLayout);
    return *this;
  }

  auto descriptorSetLayout(uint32_t index, VkDescriptorSetLayout setLayout)
      -> GraphicsPipelineDesc & {
    layout.setLayout(index, setLayout);
    return *this;
  }

  auto pushConstant(VkPushConstantRange range) -> GraphicsPipelineDesc & {
    layout.pushConstant(range);
    return *this;
  }

  auto pushConstant(VkShaderStageFlags stageFlags, uint32_t offset,
                    uint32_t size) -> GraphicsPipelineDesc & {
    layout.pushConstant(stageFlags, offset, size);
    return *this;
  }

  auto shader(GraphicsShaderStageDesc shaderDesc) -> GraphicsPipelineDesc & {
    shaders.push_back(std::move(shaderDesc));
    return *this;
  }

  auto shader(VkShaderStageFlagBits stage,
              resource::ShaderModuleDesc shaderDesc,
              std::string entryPoint = "main") -> GraphicsPipelineDesc & {
    shaders.push_back(GraphicsShaderStageDesc::make(
        stage, std::move(shaderDesc), std::move(entryPoint)));
    return *this;
  }

  auto shadersList(std::vector<GraphicsShaderStageDesc> shaderDescs)
      -> GraphicsPipelineDesc & {
    shaders = std::move(shaderDescs);
    return *this;
  }

  auto clearShaders() noexcept -> GraphicsPipelineDesc & {
    shaders.clear();
    return *this;
  }

  auto vertexShader(resource::ShaderModuleDesc shaderDesc,
                    std::string entryPoint = "main") -> GraphicsPipelineDesc & {
    shaders.push_back(GraphicsShaderStageDesc::vertex(std::move(shaderDesc),
                                                      std::move(entryPoint)));
    return *this;
  }

  auto fragmentShader(resource::ShaderModuleDesc shaderDesc,
                      std::string entryPoint = "main")
      -> GraphicsPipelineDesc & {
    shaders.push_back(GraphicsShaderStageDesc::fragment(std::move(shaderDesc),
                                                        std::move(entryPoint)));
    return *this;
  }

  auto geometryShader(resource::ShaderModuleDesc shaderDesc,
                      std::string entryPoint = "main")
      -> GraphicsPipelineDesc & {
    shaders.push_back(GraphicsShaderStageDesc::geometry(std::move(shaderDesc),
                                                        std::move(entryPoint)));
    return *this;
  }

  auto tessControlShader(resource::ShaderModuleDesc shaderDesc,
                         std::string entryPoint = "main")
      -> GraphicsPipelineDesc & {
    shaders.push_back(GraphicsShaderStageDesc::tessControl(
        std::move(shaderDesc), std::move(entryPoint)));
    return *this;
  }

  auto tessEvaluationShader(resource::ShaderModuleDesc shaderDesc,
                            std::string entryPoint = "main")
      -> GraphicsPipelineDesc & {
    shaders.push_back(GraphicsShaderStageDesc::tessEvaluation(
        std::move(shaderDesc), std::move(entryPoint)));
    return *this;
  }

  auto inputAssemblyDesc(GraphicsInputAssemblyDesc desc)
      -> GraphicsPipelineDesc & {
    inputAssembly = desc;
    return *this;
  }

  auto topology(VkPrimitiveTopology primitiveTopology) noexcept
      -> GraphicsPipelineDesc & {
    inputAssembly.primitiveTopology(primitiveTopology);
    return *this;
  }

  auto primitiveRestart(bool enabled = true) noexcept
      -> GraphicsPipelineDesc & {
    inputAssembly.primitiveRestart(enabled);
    return *this;
  }

  auto pointList() noexcept -> GraphicsPipelineDesc & {
    inputAssembly.points();
    return *this;
  }

  auto lineList() noexcept -> GraphicsPipelineDesc & {
    inputAssembly.lines();
    return *this;
  }

  auto lineStrip(bool restart = false) noexcept -> GraphicsPipelineDesc & {
    inputAssembly.lines(true, restart);
    return *this;
  }

  auto triangleList() noexcept -> GraphicsPipelineDesc & {
    inputAssembly.triangles();
    return *this;
  }

  auto triangleStrip(bool restart = false) noexcept -> GraphicsPipelineDesc & {
    inputAssembly.triangles(true, restart);
    return *this;
  }

  auto patchList(uint32_t controlPoints) noexcept -> GraphicsPipelineDesc & {
    inputAssembly.patches();
    tessellation.controlPoints(controlPoints);
    return *this;
  }

  auto tessellationDesc(GraphicsTessellationDesc desc) noexcept
      -> GraphicsPipelineDesc & {
    tessellation = desc;
    return *this;
  }

  auto viewportDesc(GraphicsViewportDesc desc) noexcept
      -> GraphicsPipelineDesc & {
    viewport = desc;
    return *this;
  }

  auto viewports(uint32_t count) noexcept -> GraphicsPipelineDesc & {
    viewport.viewports(count);
    return *this;
  }

  auto scissors(uint32_t count) noexcept -> GraphicsPipelineDesc & {
    viewport.scissors(count);
    return *this;
  }

  auto viewportCounts(uint32_t viewportCount, uint32_t scissorCount) noexcept
      -> GraphicsPipelineDesc & {
    viewport.counts(viewportCount, scissorCount);
    return *this;
  }

  auto depth(VkBool32 testEnable = VK_TRUE, VkBool32 writeEnable = VK_TRUE,
             VkCompareOp compareOp = VK_COMPARE_OP_LESS)
      -> GraphicsPipelineDesc & {
    depthStencil.depthTestEnable = testEnable;
    depthStencil.depthWriteEnable = writeEnable;
    depthStencil.depthCompareOp = compareOp;
    return *this;
  }

  auto depthStencilDesc(GraphicsDepthStencilDesc desc) noexcept
      -> GraphicsPipelineDesc & {
    depthStencil = desc;
    return *this;
  }

  auto disableDepth() -> GraphicsPipelineDesc & {
    depthStencil = GraphicsDepthStencilDesc::disabled();
    return *this;
  }

  auto readOnlyDepth() -> GraphicsPipelineDesc & {
    depthStencil = GraphicsDepthStencilDesc::readOnly();
    return *this;
  }

  auto reverseZDepth(bool writeEnable = true) -> GraphicsPipelineDesc & {
    depthStencil.reverseZ(writeEnable);
    return *this;
  }

  auto shadowMapDepth(VkCompareOp compareOp = VK_COMPARE_OP_LESS)
      -> GraphicsPipelineDesc & {
    depthStencil = GraphicsDepthStencilDesc::shadowMap(compareOp);
    return *this;
  }

  auto stencil(bool enabled = true) noexcept -> GraphicsPipelineDesc & {
    depthStencil.stencil(enabled);
    return *this;
  }

  auto stencilBoth(VkStencilOpState state) noexcept -> GraphicsPipelineDesc & {
    depthStencil.stencilBoth(state);
    return *this;
  }

  auto rasterize(GraphicsRasterizationDesc desc) -> GraphicsPipelineDesc & {
    rasterization = desc;
    return *this;
  }

  auto rasterizationDesc(GraphicsRasterizationDesc desc)
      -> GraphicsPipelineDesc & {
    rasterization = desc;
    return *this;
  }

  auto noCull() -> GraphicsPipelineDesc & {
    rasterization = GraphicsRasterizationDesc::noCull();
    return *this;
  }

  auto cull(VkCullModeFlags mode) -> GraphicsPipelineDesc & {
    rasterization.cullMode = mode;
    return *this;
  }

  auto cullBack() -> GraphicsPipelineDesc & {
    return cull(VK_CULL_MODE_BACK_BIT);
  }

  auto cullFront() -> GraphicsPipelineDesc & {
    return cull(VK_CULL_MODE_FRONT_BIT);
  }

  auto frontFace(VkFrontFace face) -> GraphicsPipelineDesc & {
    rasterization.frontFace = face;
    return *this;
  }

  auto clockwiseFrontFace() -> GraphicsPipelineDesc & {
    rasterization.clockwise();
    return *this;
  }

  auto counterClockwiseFrontFace() -> GraphicsPipelineDesc & {
    rasterization.counterClockwise();
    return *this;
  }

  auto polygonMode(VkPolygonMode mode) -> GraphicsPipelineDesc & {
    rasterization.polygon(mode);
    return *this;
  }

  auto fill() -> GraphicsPipelineDesc & {
    rasterization.fill();
    return *this;
  }

  auto wireframe(float lineWidth = 1.0f) -> GraphicsPipelineDesc & {
    rasterization.wireframe().line(lineWidth);
    return *this;
  }

  auto rasterPoints(float pointLineWidth = 1.0f) -> GraphicsPipelineDesc & {
    rasterization.points().line(pointLineWidth);
    return *this;
  }

  auto depthBias(float constantFactor, float slopeFactor, float clamp = 0.0f)
      -> GraphicsPipelineDesc & {
    rasterization.depthBias(constantFactor, slopeFactor, clamp);
    return *this;
  }

  auto
  shadowRasterization(float constantFactor = 1.25f, float slopeFactor = 1.75f,
                      VkCullModeFlags shadowCullMode = VK_CULL_MODE_BACK_BIT,
                      float clamp = 0.0f) -> GraphicsPipelineDesc & {
    rasterization = GraphicsRasterizationDesc::shadow(
        constantFactor, slopeFactor, shadowCullMode, clamp);
    return *this;
  }

  auto multisampleDesc(GraphicsMultisampleDesc desc) noexcept
      -> GraphicsPipelineDesc & {
    multisample = std::move(desc);
    return *this;
  }

  auto samples(VkSampleCountFlagBits count) noexcept -> GraphicsPipelineDesc & {
    multisample.samples(count);
    return *this;
  }

  auto sampleShading(bool enabled = true, float minSample = 1.0f) noexcept
      -> GraphicsPipelineDesc & {
    multisample.sampleShading(enabled, minSample);
    return *this;
  }

  auto alphaToCoverage(bool enabled = true) noexcept -> GraphicsPipelineDesc & {
    multisample.alphaToCoverage(enabled);
    return *this;
  }

  auto blend(GraphicsColorBlendDesc desc) -> GraphicsPipelineDesc & {
    colorBlend = std::move(desc);
    return *this;
  }

  auto alphaBlend() -> GraphicsPipelineDesc & {
    colorBlend = GraphicsColorBlendDesc::alphaBlend();
    return *this;
  }

  auto opaqueBlend(uint32_t attachmentCount = 1) -> GraphicsPipelineDesc & {
    colorBlend.opaque(attachmentCount);
    return *this;
  }

  auto premultipliedAlphaBlend(uint32_t attachmentCount = 1)
      -> GraphicsPipelineDesc & {
    colorBlend.premultipliedAlpha(attachmentCount);
    return *this;
  }

  auto additiveBlend(uint32_t attachmentCount = 1) -> GraphicsPipelineDesc & {
    colorBlend.additive(attachmentCount);
    return *this;
  }

  auto colorWriteMask(VkColorComponentFlags mask, uint32_t index = 0)
      -> GraphicsPipelineDesc & {
    colorBlend.colorWriteMask(mask, index);
    return *this;
  }

  auto disableColorWrite(uint32_t index = 0) -> GraphicsPipelineDesc & {
    colorBlend.colorWriteMask(0, index);
    return *this;
  }

  auto depthOnlyColor() -> GraphicsPipelineDesc & {
    colorBlend = GraphicsColorBlendDesc::depthOnly();
    return *this;
  }

  auto dynamicStateDesc(GraphicsDynamicStateDesc desc) noexcept
      -> GraphicsPipelineDesc & {
    dynamicState = std::move(desc);
    return *this;
  }

  auto dynamic(VkDynamicState state) -> GraphicsPipelineDesc & {
    dynamicState.state(state);
    return *this;
  }

  auto staticViewportScissor() -> GraphicsPipelineDesc & {
    dynamicState.viewport(false).scissor(false);
    return *this;
  }

  auto pipelineSubpass(uint32_t index) noexcept -> GraphicsPipelineDesc & {
    subpass = index;
    return *this;
  }

  auto base(VkPipeline pipelineHandle, int32_t pipelineIndex = -1) noexcept
      -> GraphicsPipelineDesc & {
    basePipeline = pipelineHandle;
    basePipelineIndex = pipelineIndex;
    return *this;
  }

  auto derivative(bool enabled = true) noexcept -> GraphicsPipelineDesc & {
    if (enabled) {
      flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    } else {
      flags &= ~VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    }
    return *this;
  }

  [[nodiscard]] static auto empty() -> GraphicsPipelineDesc { return {}; }

  [[nodiscard]] static auto named(std::string pipelineName)
      -> GraphicsPipelineDesc {
    GraphicsPipelineDesc desc{};
    return desc.setName(std::move(pipelineName));
  }

  [[nodiscard]] static auto fullscreen(std::string pipelineName)
      -> GraphicsPipelineDesc {
    GraphicsPipelineDesc desc{};
    return desc.setName(std::move(pipelineName))
        .vertexInputDesc(scene::VertexInputDesc::none())
        .triangleList()
        .disableDepth()
        .noCull();
  }

  [[nodiscard]] static auto mesh(std::string pipelineName,
                                 scene::VertexInputDesc vertexInputDesc)
      -> GraphicsPipelineDesc {
    GraphicsPipelineDesc desc{};
    return desc.setName(std::move(pipelineName))
        .vertexInputDesc(std::move(vertexInputDesc))
        .triangleList()
        .depth()
        .cullBack();
  }

  [[nodiscard]] static auto
  wireframePreset(std::string pipelineName,
                  scene::VertexInputDesc vertexInputDesc,
                  float lineWidth = 1.0f) -> GraphicsPipelineDesc {
    GraphicsPipelineDesc desc{};
    return desc.setName(std::move(pipelineName))
        .vertexInputDesc(std::move(vertexInputDesc))
        .triangleList()
        .depth()
        .wireframe(lineWidth)
        .noCull();
  }

  [[nodiscard]] static auto
  shadowMap(std::string pipelineName, scene::VertexInputDesc vertexInputDesc,
            float depthBiasConstant = 1.25f, float depthBiasSlope = 1.75f,
            VkCullModeFlags shadowCullMode = VK_CULL_MODE_BACK_BIT,
            VkCompareOp compareOp = VK_COMPARE_OP_LESS)
      -> GraphicsPipelineDesc {
    GraphicsPipelineDesc desc{};
    return desc.setName(std::move(pipelineName))
        .vertexInputDesc(std::move(vertexInputDesc))
        .triangleList()
        .shadowMapDepth(compareOp)
        .shadowRasterization(depthBiasConstant, depthBiasSlope, shadowCullMode)
        .depthOnlyColor();
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    if (shaders.empty() || name.empty()) {
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
  GraphicsPipeline(const core::Device &device, const RenderPass &renderPass);
  ~GraphicsPipeline();

  GraphicsPipeline(const GraphicsPipeline &) = delete;
  auto operator=(const GraphicsPipeline &) -> GraphicsPipeline & = delete;

  void destroy();
  auto update(const GraphicsPipelineDesc &desc) -> bool;

  [[nodiscard]] auto desc() const noexcept -> const GraphicsPipelineDesc & {
    return desc_;
  }

  [[nodiscard]] auto shaders() const noexcept
      -> const std::vector<GraphicsShaderStageDesc> & {
    return desc_.shaders;
  }

  [[nodiscard]] auto shaderModules() const noexcept
      -> const std::vector<std::unique_ptr<resource::ShaderModule>> & {
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

  [[nodiscard]] auto revision() const noexcept -> uint64_t { return revision_; }

private:
  struct RetiredPipeline {
    VkPipeline pipeline{VK_NULL_HANDLE};
    VkPipelineLayout layout{VK_NULL_HANDLE};
  };

  // dependencies
  const core::Device &device_;
  const RenderPass &render_pass_;

  // components
  GraphicsPipelineDesc desc_{};
  std::vector<std::unique_ptr<resource::ShaderModule>> shader_modules_{};
  VkPipelineLayout vk_pipeline_layout_{VK_NULL_HANDLE};
  VkPipeline vk_graphics_pipeline_{VK_NULL_HANDLE};
  std::vector<RetiredPipeline> retired_pipelines_{};
  uint64_t revision_{0};
};

} // namespace vkr::pipeline
