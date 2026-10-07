#include "vkr/exec/render/passes/overlay.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <unordered_set>
#include <utility>

namespace vkr::exec {

auto OverlayPassDesc::wireframe(pipeline::GraphicsPipelineDesc pipelineDesc)
    -> OverlayPassDesc {
  pipelineDesc.name += "-wireframe";
  pipelineDesc.shaders.erase(
      std::remove_if(pipelineDesc.shaders.begin(), pipelineDesc.shaders.end(),
                     [](const auto &shader) -> bool {
                       return shader.stage == VK_SHADER_STAGE_FRAGMENT_BIT;
                     }),
      pipelineDesc.shaders.end());
  pipelineDesc.fragmentShader(resource::ShaderModuleDesc::fragmentGlslSource(
      R"glsl(#version 450
layout(location = 0) out vec4 outColor;

void main() {
  outColor = vec4(1.0, 0.75, 0.1, 1.0);
}
)glsl",
      pipelineDesc.name + ".frag"));
  pipelineDesc.inputAssembly.lines();
  pipelineDesc.rasterization = pipeline::GraphicsRasterizationDesc::noCull();
  pipelineDesc.depthStencil = pipeline::GraphicsDepthStencilDesc::readOnly();
  pipelineDesc.colorBlend.opaque().disableLogic();

  OverlayPassDesc desc{};
  desc.pipeline = std::move(pipelineDesc);
  return desc;
}

OverlayPass::OverlayPass(RenderExecutor &executor, const core::Device &device,
                         const core::CommandPool &commandPool,
                         scene::Scene &scene, Pass &source)
    : executor_(executor), device_(device), command_pool_(commandPool),
      scene_(scene), source_(source), descriptor_pool_(device),
      descriptor_layout_(device),
      descriptor_sets_(device, descriptor_pool_, descriptor_layout_) {}

OverlayPass::~OverlayPass() { destroy(); }

void OverlayPass::update(const OverlayPassDesc &desc) {
  ensureConfigurable();
  if (render_pass_ || pipeline_ || descriptor_layout_.valid()) {
    VKR_EXEC_ERROR("OverlayPass '{}' must be destroyed before updating its "
                   "configuration",
                   name());
  }
  validate(desc);
  auto nextDesc = desc;
  desc_ = std::move(nextDesc);
}

void OverlayPass::create() {
  validate(desc_);
  destroy();

  target_source_ = source_.capability<RenderTargetCapability>();
  if (!target_source_) {
    VKR_EXEC_ERROR("OverlayPass '{}' source '{}' has no render target", name(),
                   source_.name());
  }

  createRenderPass();
  createFramebuffers();
  createDescriptors();
  createPipeline();
  createMeshes();
}

void OverlayPass::destroy() noexcept {
  selected_mesh_.reset();
  meshes_.clear();
  pipeline_.reset();
  descriptor_sets_.destroy();
  descriptor_layout_.destroy();
  descriptor_pool_.destroy();
  framebuffers_.reset();
  render_pass_.reset();
  target_source_.reset();
}

void OverlayPass::selectMesh(const std::string &name) noexcept {
  const auto mesh = meshes_.find(name);
  if (mesh == meshes_.end()) {
    selected_mesh_.reset();
  } else {
    selected_mesh_ = std::cref(mesh->second);
  }
}

auto OverlayPass::target(uint32_t frameIndex) -> OffscreenTarget & {
  if (!target_source_) {
    VKR_EXEC_ERROR("OverlayPass '{}' target requested before create", name());
  }
  return target_source_->get().target(frameIndex);
}

auto OverlayPass::target(uint32_t frameIndex) const -> const OffscreenTarget & {
  if (!target_source_) {
    VKR_EXEC_ERROR("OverlayPass '{}' target requested before create", name());
  }
  return std::as_const(target_source_->get()).target(frameIndex);
}

void OverlayPass::record() {
  if (!pipeline_ || !framebuffers_) {
    VKR_EXEC_ERROR("OverlayPass '{}' recorded before create", name());
  }
  if (!selected_mesh_) {
    return;
  }

  const auto &mesh = selected_mesh_->get();
  executor_.beginProfileScope(name());
  executor_.beginPass(*framebuffers_, {}, executor_.frameIndex());
  executor_.setViewportAndScissor(framebuffers_->extent());
  descriptor_sets_.write(executor_.frameIndex());
  if (descriptor_sets_.valid()) {
    executor_.bindPipeline(*pipeline_, descriptor_sets_);
  } else {
    executor_.bindPipeline(*pipeline_);
  }
  executor_.drawIndexed(mesh.vertices.get(), *mesh.indices);
  executor_.endPass();
  executor_.endProfileScope();
}

void OverlayPass::createRenderPass() {
  const auto &output = target(0);
  if (!output.hasColor()) {
    VKR_EXEC_ERROR("OverlayPass '{}' requires a color target", name());
  }

  const auto &outputDesc = output.desc();
  const auto &depthState = desc_.pipeline.depthStencil;
  const bool useDepth = depthState.depthTestEnable ||
                        depthState.depthWriteEnable ||
                        depthState.stencilTestEnable;
  if (useDepth &&
      (!output.hasDepth() || !outputDesc.depth ||
       (outputDesc.depth->storeOp != VK_ATTACHMENT_STORE_OP_STORE &&
        (outputDesc.depth->usage & VK_IMAGE_USAGE_SAMPLED_BIT) == 0))) {
    VKR_EXEC_ERROR("OverlayPass '{}' requires preserved producer depth; "
                   "configure store(VK_ATTACHMENT_STORE_OP_STORE)",
                   name());
  }

  auto passDesc = pipeline::RenderPassDesc::makeOffscreen(
      outputDesc.color.format,
      useDepth ? outputDesc.depth->format : VK_FORMAT_UNDEFINED);
  auto colorLayout = outputDesc.color.finalLayout;
  if (colorLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
    colorLayout =
        (outputDesc.color.usage & (VK_IMAGE_USAGE_SAMPLED_BIT |
                                   VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)) != 0
            ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  }
  passDesc.colors[0]
      .load(VK_ATTACHMENT_LOAD_OP_LOAD)
      .layouts(colorLayout, colorLayout);
  if (useDepth) {
    auto depthLayout = outputDesc.depth->finalLayout;
    if (depthLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
      depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    passDesc.depth.load(VK_ATTACHMENT_LOAD_OP_LOAD)
        .store(VK_ATTACHMENT_STORE_OP_STORE)
        .layouts(depthLayout, depthLayout);
    if (depthState.stencilTestEnable) {
      VKR_EXEC_ERROR("OverlayPass '{}' does not preserve producer stencil",
                     name());
    }
  }

  VkPipelineStageFlags attachmentStages =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkAccessFlags attachmentAccess = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  if (useDepth) {
    attachmentStages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    attachmentAccess |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  }
  passDesc.dependencies = {
      {.srcSubpass = VK_SUBPASS_EXTERNAL,
       .dstSubpass = 0,
       .srcStageMask = attachmentStages | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
       .dstStageMask = attachmentStages,
       .srcAccessMask = attachmentAccess | VK_ACCESS_SHADER_READ_BIT,
       .dstAccessMask = attachmentAccess},
      {.srcSubpass = 0,
       .dstSubpass = VK_SUBPASS_EXTERNAL,
       .srcStageMask = attachmentStages,
       .dstStageMask = attachmentStages | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
       .srcAccessMask = attachmentAccess,
       .dstAccessMask = attachmentAccess | VK_ACCESS_SHADER_READ_BIT}};
  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  render_pass_->update(passDesc);
}

void OverlayPass::createFramebuffers() {
  const auto &first = target(0);
  FramebufferDesc desc{};
  desc.extent(first.width(), first.height());
  const bool useDepth = render_pass_->desc().hasDepth();
  for (uint32_t frame = 0; frame < executor_.framesInFlight(); ++frame) {
    const auto &output = target(frame);
    if (!output.hasColor() || output.width() != first.width() ||
        output.height() != first.height() ||
        output.desc().color.format != first.desc().color.format ||
        (useDepth &&
         (!output.hasDepth() ||
          output.desc().depth->format != first.desc().depth->format ||
          output.desc().depth->finalLayout != first.desc().depth->finalLayout ||
          (output.desc().depth->storeOp != VK_ATTACHMENT_STORE_OP_STORE &&
           (output.desc().depth->usage & VK_IMAGE_USAGE_SAMPLED_BIT) == 0)))) {
      VKR_EXEC_ERROR("OverlayPass '{}' target differs between frames", name());
    }
    if (useDepth) {
      desc.attachmentViews(output.attachmentViews());
    } else {
      desc.attachmentViews(
          std::vector<VkImageView>{output.color().imageView()});
    }
  }
  framebuffers_ = std::make_unique<FramebufferSet>(device_, *render_pass_);
  framebuffers_->update(desc);
}

void OverlayPass::validate(const OverlayPassDesc &desc) const {
  if (desc.meshNames.empty() || !desc.pipeline.isValid() ||
      desc.pipeline.inputAssembly.topology !=
          VK_PRIMITIVE_TOPOLOGY_LINE_LIST) {
    VKR_EXEC_ERROR("OverlayPass '{}' requires meshes and a valid line pipeline",
                   name());
  }

  VkShaderStageFlags stages{};
  for (const auto &shader : desc.pipeline.shaders) {
    if (shader.stage != VK_SHADER_STAGE_VERTEX_BIT &&
        shader.stage != VK_SHADER_STAGE_FRAGMENT_BIT) {
      VKR_EXEC_ERROR("OverlayPass '{}' supports only vertex and fragment "
                     "shader stages; received stage {}",
                     name(), static_cast<int>(shader.stage));
    }
    if ((stages & shader.stage) != 0) {
      VKR_EXEC_ERROR("OverlayPass '{}' has duplicate shader stage {}", name(),
                     static_cast<int>(shader.stage));
    }
    stages |= shader.stage;
  }
  if (stages != (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)) {
    VKR_EXEC_ERROR("OverlayPass '{}' requires one vertex and one fragment "
                   "shader stage",
                   name());
  }
  if (desc.pipeline.inputAssembly.primitiveRestartEnable) {
    VKR_EXEC_ERROR("OverlayPass '{}' line indices do not use primitive restart",
                   name());
  }

  pipeline::DescriptorSetLayoutDesc layoutDesc{
      .bindings = desc.descriptorBindings};
  if (!layoutDesc.isValid()) {
    VKR_EXEC_ERROR("OverlayPass '{}' requires unique bindings with nonzero "
                   "counts/stages",
                   name());
  }
  for (const auto &binding : layoutDesc.bindings) {
    const auto &layout = binding.layout;
    if (layout.descriptorCount != 1 ||
        (layout.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
         layout.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
         layout.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) {
      VKR_EXEC_ERROR("OverlayPass '{}' has an unsupported descriptor binding {}",
                     name(), layout.binding);
    }
  }
}

void OverlayPass::createDescriptors() {
  if (desc_.descriptorBindings.empty()) {
    descriptor_sets_.update({.setCount = 0});
    return;
  }
  const pipeline::DescriptorSetLayoutDesc layoutDesc{
      .bindings = desc_.descriptorBindings};
  descriptor_pool_.update(pipeline::DescriptorPoolDesc::sets(
      layoutDesc, executor_.framesInFlight()));
  descriptor_layout_.update(layoutDesc);
  descriptor_sets_.update({.setCount = executor_.framesInFlight()});
  for (uint32_t index = 0; index < descriptor_sets_.count(); ++index) {
    descriptor_sets_.write(index);
  }
}

void OverlayPass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;
  if (descriptor_layout_.valid() && pipelineDesc.layout.setLayouts.empty()) {
    pipelineDesc.layout.setLayouts = {descriptor_layout_.layout()};
  }
  pipeline_ =
      std::make_unique<pipeline::GraphicsPipeline>(device_, *render_pass_);
  pipeline_->update(pipelineDesc);
  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR("OverlayPass '{}' failed to create pipeline", name());
  }
}

void OverlayPass::createMeshes() {
  meshes_.reserve(desc_.meshNames.size());
  for (const auto &name : desc_.meshNames) {
    if (meshes_.find(name) != meshes_.end()) {
      VKR_EXEC_ERROR("OverlayPass '{}' has duplicate mesh '{}'", this->name(),
                     name);
    }
    const auto &mesh = scene_.mesh(name);
    const auto vertices = mesh.vertexBufferBase();
    const auto indices = mesh.indexBuffer();
    if (!vertices || !indices) {
      VKR_EXEC_ERROR("OverlayPass '{}' mesh '{}' has invalid buffers",
                     this->name(), name);
    }
    const auto input = vertices->get().vertexInputDesc();
    const auto &expected = desc_.pipeline.vertexInput;
    const bool sameBindings =
        input.bindings.size() == expected.bindings.size() &&
        std::equal(input.bindings.begin(), input.bindings.end(),
                   expected.bindings.begin(),
                   [](const auto &a, const auto &b) -> bool {
                     return a.binding == b.binding && a.stride == b.stride &&
                            a.inputRate == b.inputRate;
                   });
    const bool sameAttributes =
        input.attributes.size() == expected.attributes.size() &&
        std::equal(input.attributes.begin(), input.attributes.end(),
                   expected.attributes.begin(),
                   [](const auto &a, const auto &b) -> bool {
                     return a.location == b.location &&
                            a.binding == b.binding && a.format == b.format &&
                            a.offset == b.offset;
                   });
    if (!sameBindings || !sameAttributes || input.bindings.size() != 1 ||
        input.bindings[0].binding != 0 ||
        input.bindings[0].inputRate != VK_VERTEX_INPUT_RATE_VERTEX) {
      VKR_EXEC_ERROR("OverlayPass '{}' mesh '{}' has incompatible vertex input",
                     this->name(), name);
    }
    const auto &triangles = indices->get().indices();
    if (triangles.empty() || triangles.size() % 3 != 0) {
      VKR_EXEC_ERROR("OverlayPass '{}' mesh '{}' is not a triangle list",
                     this->name(), name);
    }

    std::vector<uint16_t> lines{};
    lines.reserve(triangles.size() * 2);
    std::unordered_set<uint32_t> edges{};
    edges.reserve(triangles.size());
    for (size_t triangle = 0; triangle < triangles.size(); triangle += 3) {
      for (size_t edge = 0; edge < 3; ++edge) {
        const auto a = triangles[triangle + edge];
        const auto b = triangles[triangle + (edge + 1) % 3];
        if (a >= vertices->get().vertexCount() ||
            b >= vertices->get().vertexCount()) {
          VKR_EXEC_ERROR("OverlayPass '{}' mesh '{}' index is out of range",
                         this->name(), name);
        }
        const auto lo = std::min(a, b);
        const auto hi = std::max(a, b);
        const uint32_t key = (static_cast<uint32_t>(lo) << 16) | hi;
        if (lo != hi && edges.insert(key).second) {
          lines.push_back(a);
          lines.push_back(b);
        }
      }
    }
    if (lines.empty()) {
      VKR_EXEC_ERROR("OverlayPass '{}' mesh '{}' has no drawable edges",
                     this->name(), name);
    }
    auto buffer = std::make_unique<scene::IndexBuffer>(device_, command_pool_);
    buffer->update(lines);
    meshes_.emplace(name, MeshEntry{vertices->get(), std::move(buffer)});
  }
}

} // namespace vkr::exec
