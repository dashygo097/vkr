#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/io_utils.hh"
#include "vkr/logger.hh"
#include "vkr/pipeline/shader_module.hh"
#include "vkr/resources/buffers/vertex_buffer.hh"
#include <filesystem>
#include <shaderc/shaderc.hpp>

namespace vkr::pipeline {

GraphicsPipeline::GraphicsPipeline(
    const core::Instance &instance, const core::Device &device,
    const resource::ResourceManager &resourceManager,
    const RenderPass &renderPass, DescriptorSetLayout &descriptorSetLayout,
    PipelineMode mode)
    : instance_(instance), device_(device), resource_manager_(resourceManager),
      render_pass_(renderPass), descriptor_set_layout_(descriptorSetLayout),
      mode_(mode) {
  VKR_PIPE_INFO("Graphics pipeline initializing...");

  loadDefaultSources();
  std::string vertErr = "Default vertex shader compilation failed!";
  std::string fragErr = "Default fragment shader compilation failed!";

  auto vertSpv = compileGlsl(vert_src_, true, vertErr);
  auto fragSpv = compileGlsl(frag_src_, false, fragErr);

  if (!build(vertSpv, fragSpv))
    VKR_PIPE_ERROR("Failed to build graphics pipeline from default sources!");

  VKR_PIPE_INFO("Graphics pipeline initialized!");
}

GraphicsPipeline::~GraphicsPipeline() {
  destroyOffscreenHandles();
  destroyHandles();
}

bool GraphicsPipeline::buildInto(const std::vector<uint32_t> &vertSpv,
                                 const std::vector<uint32_t> &fragSpv,
                                 VkRenderPass targetRenderPass,
                                 VkPipelineLayout &outLayout,
                                 VkPipeline &outPipeline) {
  ShaderModule vertShader(device_, vertSpv);
  ShaderModule fragShader(device_, fragSpv);

  VkPipelineShaderStageCreateInfo vertStage{};
  vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertStage.module = vertShader.module();
  vertStage.pName = "main";

  VkPipelineShaderStageCreateInfo fragStage{};
  fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragStage.module = fragShader.module();
  fragStage.pName = "main";

  VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  VkVertexInputBindingDescription bindingDescription;
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

  switch (mode_) {
  case PipelineMode::Default2D:
    bindingDescription = resource::Vertex2D::getBindingDescription();
    attributeDescriptions = resource::Vertex2D::getAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    break;
  case PipelineMode::Textured2D:
    bindingDescription = resource::VertexTextured2D::getBindingDescription();
    attributeDescriptions =
        resource::VertexTextured2D::getAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    break;
  case PipelineMode::Default3D:
    bindingDescription = resource::Vertex3D::getBindingDescription();
    attributeDescriptions = resource::Vertex3D::getAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    break;
  case PipelineMode::Textured3D:
    bindingDescription = resource::VertexTextured3D::getBindingDescription();
    attributeDescriptions =
        resource::VertexTextured3D::getAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    break;
  case PipelineMode::NoVertices:
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    break;
  }

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = mode_ == PipelineMode::NoVertices
                            ? VK_CULL_MODE_NONE
                            : VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  std::vector<VkDynamicState> dynStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                           VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
  dynamicState.pDynamicStates = dynStates.data();

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts = &descriptor_set_layout_.layoutRef();

  VkPipelineLayout newLayout{VK_NULL_HANDLE};
  if (vkCreatePipelineLayout(device_.device(), &layoutInfo, nullptr,
                             &newLayout) != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create pipeline layout!");
    return false;
  }

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = stages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = newLayout;
  pipelineInfo.renderPass = targetRenderPass;
  pipelineInfo.subpass = 0;

  VkPipeline newPipeline{VK_NULL_HANDLE};
  if (vkCreateGraphicsPipelines(device_.device(), VK_NULL_HANDLE, 1,
                                &pipelineInfo, nullptr,
                                &newPipeline) != VK_SUCCESS) {
    vkDestroyPipelineLayout(device_.device(), newLayout, nullptr);
    VKR_PIPE_ERROR("Failed to create graphics pipeline!");
    return false;
  }

  outLayout = newLayout;
  outPipeline = newPipeline;
  return true;
}

bool GraphicsPipeline::build(const std::vector<uint32_t> &vertSpv,
                             const std::vector<uint32_t> &fragSpv) {
  return buildInto(vertSpv, fragSpv, render_pass_.renderPass(),
                   vk_pipeline_layout_, vk_graphics_pipeline_);
}

void GraphicsPipeline::buildOffscreen(VkRenderPass offscreenRenderPass) {
  VKR_PIPE_INFO("Building offscreen pipeline...");
  offscreen_render_pass_ = offscreenRenderPass;
  destroyOffscreenHandles();

  std::string err;
  auto vertSpv = compileGlsl(vert_src_, true, err);
  if (vertSpv.empty()) {
    VKR_PIPE_ERROR("Offscreen pipeline: vertex compile failed: {}", err);
    return;
  }
  auto fragSpv = compileGlsl(frag_src_, false, err);
  if (fragSpv.empty()) {
    VKR_PIPE_ERROR("Offscreen pipeline: fragment compile failed: {}", err);
    return;
  }

  if (!buildInto(vertSpv, fragSpv, offscreenRenderPass, vk_offscreen_layout_,
                 vk_offscreen_pipeline_)) {
    VKR_PIPE_ERROR("Failed to build offscreen pipeline!");
  } else {
    VKR_PIPE_INFO("Offscreen pipeline built successfully.");
  }
}

void GraphicsPipeline::destroyOffscreenHandles() {
  if (vk_offscreen_pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_.device(), vk_offscreen_pipeline_, nullptr);
    vk_offscreen_pipeline_ = VK_NULL_HANDLE;
  }
  if (vk_offscreen_layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_.device(), vk_offscreen_layout_, nullptr);
    vk_offscreen_layout_ = VK_NULL_HANDLE;
  }
}

bool GraphicsPipeline::rebuild(const std::string &vertShaderPath,
                               const std::string &fragShaderPath) {
  VKR_PIPE_INFO("Hot-reloading pipeline from files...");

  std::vector<uint32_t> vertSpv, fragSpv;
  try {
    vertSpv = fread_uint32(vertShaderPath);
    fragSpv = fread_uint32(fragShaderPath);
  } catch (const std::exception &e) {
    VKR_PIPE_ERROR("Reload failed: {}", e.what());
    return false;
  }

  vkDeviceWaitIdle(device_.device());
  destroyHandles();
  bool ok = build(vertSpv, fragSpv);

  if (ok && offscreen_render_pass_ != VK_NULL_HANDLE)
    buildOffscreen(offscreen_render_pass_);

  VKR_PIPE_INFO("Pipeline hot-reload {}.", ok ? "succeeded" : "FAILED");
  return ok;
}

void GraphicsPipeline::requestRebuildFromSource(
    const std::string &vertSrc, const std::string &fragSrc,
    std::function<void(bool, const std::string &)> callback) {

  std::string err;
  auto vertSpv = compileGlsl(vertSrc, true, err);
  if (vertSpv.empty()) {
    callback(false, err);
    return;
  }
  auto fragSpv = compileGlsl(fragSrc, false, err);
  if (fragSpv.empty()) {
    callback(false, err);
    return;
  }

  std::lock_guard lock(pending_mutex_);
  pending_rebuild_ = PendingRebuild{vertSrc, fragSrc, std::move(vertSpv),
                                    std::move(fragSpv), std::move(callback)};
  VKR_PIPE_INFO("Rebuild staged, will apply at next frame boundary.");
}

bool GraphicsPipeline::flushPendingRebuild() {
  std::lock_guard lock(pending_mutex_);
  if (!pending_rebuild_)
    return false;

  PendingRebuild req = std::move(*pending_rebuild_);
  pending_rebuild_.reset();

  vkDeviceWaitIdle(device_.device());
  destroyHandles();
  bool ok = build(req.vertSpv, req.fragSpv);

  if (ok) {
    if (!vert_src_path_.empty())
      fwrite_string(vert_src_path_, req.vertSrc);
    if (!frag_src_path_.empty())
      fwrite_string(frag_src_path_, req.fragSrc);
    vert_src_ = req.vertSrc;
    frag_src_ = req.fragSrc;

    if (offscreen_render_pass_ != VK_NULL_HANDLE)
      buildOffscreen(offscreen_render_pass_);
  }

  req.callback(ok, ok ? "" : "Pipeline build step failed.");
  VKR_PIPE_INFO("Deferred rebuild {}.", ok ? "succeeded" : "FAILED");
  return true;
}

std::vector<uint32_t> GraphicsPipeline::compileGlsl(const std::string &src,
                                                    bool isVertex,
                                                    std::string &outError) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions opts;
  opts.SetOptimizationLevel(shaderc_optimization_level_performance);
  opts.SetTargetEnvironment(shaderc_target_env_vulkan,
                            shaderc_env_version_vulkan_1_0);

  auto kind =
      isVertex ? shaderc_glsl_vertex_shader : shaderc_glsl_fragment_shader;
  const char *label = isVertex ? "vertex" : "fragment";

  shaderc::SpvCompilationResult result =
      compiler.CompileGlslToSpv(src, kind, label, opts);

  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    outError = result.GetErrorMessage();
    return {};
  }
  return {result.cbegin(), result.cend()};
}

void GraphicsPipeline::destroyHandles() {
  if (vk_graphics_pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_.device(), vk_graphics_pipeline_, nullptr);
    vk_graphics_pipeline_ = VK_NULL_HANDLE;
  }
  if (vk_pipeline_layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_.device(), vk_pipeline_layout_, nullptr);
    vk_pipeline_layout_ = VK_NULL_HANDLE;
  }
}

static const std::unordered_map<PipelineMode,
                                std::pair<std::string, std::string>>
    kDefaultSourcePaths = {
        {PipelineMode::Default2D,
         {"shaders/default2d/default2d.vert",
          "shaders/default2d/default2d.frag"}},
        {PipelineMode::Textured2D,
         {"shaders/texture2d/texture2d.vert",
          "shaders/texture2d/texture2d.frag"}},
        {PipelineMode::Default3D,
         {"shaders/default3d/default3d.vert",
          "shaders/default3d/default3d.frag"}},
        {PipelineMode::Textured3D,
         {"shaders/texture3d/texture3d.vert",
          "shaders/texture3d/texture3d.frag"}},
        {PipelineMode::NoVertices,
         {"shaders/shadertoy/shadertoy.vert",
          "shaders/shadertoy/shadertoy.frag"}},
};

void GraphicsPipeline::loadDefaultSources() {
  namespace fs = std::filesystem;

  auto it = kDefaultSourcePaths.find(mode_);
  if (it == kDefaultSourcePaths.end()) {
    VKR_PIPE_WARN(
        "No default source paths found for the current PipelineMode.");
    return;
  }
  const auto &[defaultVert, defaultFrag] = it->second;

  if (!instance_.name().empty()) {
    std::string appDir = "shaders/" + instance_.name();
    std::string appVert = appDir + "/" + instance_.name() + ".vert";
    std::string appFrag = appDir + "/" + instance_.name() + ".frag";

    if (!fs::exists(appVert) || !fs::exists(appFrag)) {
      std::error_code ec;
      fs::create_directories(appDir, ec);
      if (ec) {
        VKR_PIPE_WARN("Could not create shader directory '{}': {} — "
                      "falling back to read-only mode defaults.",
                      appDir, ec.message());
      } else {
        fs::copy_file(defaultVert, appVert, fs::copy_options::skip_existing,
                      ec);
        if (ec)
          VKR_PIPE_WARN("Could not copy '{}' → '{}': {}", defaultVert, appVert,
                        ec.message());

        fs::copy_file(defaultFrag, appFrag, fs::copy_options::skip_existing,
                      ec);
        if (ec)
          VKR_PIPE_WARN("Could not copy '{}' → '{}': {}", defaultFrag, appFrag,
                        ec.message());

        VKR_PIPE_INFO("Created app shaders from defaults: {} / {}", appVert,
                      appFrag);
      }
    }

    if (fs::exists(appVert) && fs::exists(appFrag)) {
      vert_src_path_ = appVert;
      frag_src_path_ = appFrag;
      vert_src_ = fread_string(appVert);
      frag_src_ = fread_string(appFrag);
      VKR_PIPE_INFO("Loaded app shaders: {} / {}", appVert, appFrag);
      return;
    }

    VKR_PIPE_WARN("App shaders still unavailable at {} / {} — "
                  "falling back to read-only mode defaults.",
                  appVert, appFrag);
  }

  vert_src_path_ = "";
  frag_src_path_ = "";
  vert_src_ = fread_string(defaultVert);
  frag_src_ = fread_string(defaultFrag);
  VKR_PIPE_INFO("Loaded mode-default shaders: {} / {} (read-only)", defaultVert,
                defaultFrag);
}

} // namespace vkr::pipeline
