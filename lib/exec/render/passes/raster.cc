#include "vkr/exec/render/passes/raster.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <string_view>
#include <unordered_set>

namespace vkr::exec {
namespace {

auto imageLayoutForAttachment(const ColorAttachment &attachment)
    -> VkImageLayout {
  return attachment.desc().finalLayout == VK_IMAGE_LAYOUT_UNDEFINED
             ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
             : attachment.desc().finalLayout;
}

auto sourceImageInfo(std::string_view passName, size_t sourceIndex,
                     const RenderPassSource &source,
                     const RenderPassInputDesc &input) -> VkDescriptorImageInfo {
  VkDescriptorImageInfo imageInfo{};

  switch (input.kind) {
  case RenderPassInputKind::Color: {
    const auto &color = source.target().color();
    if (!color.hasSampler()) {
      VKR_EXEC_ERROR("RasterPass '{}' source {} color has no sampler",
                     std::string(passName), sourceIndex);
    }

    imageInfo.imageLayout = imageLayoutForAttachment(color);
    imageInfo.imageView = color.imageView();
    imageInfo.sampler = color.sampler();
    break;
  }

  case RenderPassInputKind::Depth: {
    const auto *depth = source.target().depth();
    if (depth == nullptr) {
      VKR_EXEC_ERROR("RasterPass '{}' source {} has no depth attachment",
                     std::string(passName), sourceIndex);
    }

    if (!depth->hasSampler()) {
      VKR_EXEC_ERROR("RasterPass '{}' source {} depth has no sampler",
                     std::string(passName), sourceIndex);
    }

    imageInfo.imageLayout =
        depth->desc().finalLayout == VK_IMAGE_LAYOUT_UNDEFINED
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            : depth->desc().finalLayout;
    imageInfo.imageView = depth->imageView();
    imageInfo.sampler = depth->sampler();
    break;
  }
  }

  return imageInfo;
}

void validateUniqueDescriptorBindings(
    std::string_view passName,
    const std::vector<pipeline::DescriptorBinding> &bindings) {
  for (size_t i = 0; i < bindings.size(); ++i) {
    for (size_t j = i + 1; j < bindings.size(); ++j) {
      if (bindings[i].layout.binding == bindings[j].layout.binding) {
        VKR_EXEC_ERROR("RasterPass '{}' has duplicate descriptor binding {}",
                       std::string(passName), bindings[i].layout.binding);
      }
    }
  }
}

} // namespace

RasterPass::RasterPass(Executor &executor, const core::Device &device,
                       const core::CommandPool &commandPool,
                       scene::Scene &scene)
    : executor_(executor), device_(device), command_pool_(commandPool),
      scene_(scene) {}

RasterPass::~RasterPass() { destroy(); }

void RasterPass::create() {
  destroy();

  createTarget();
  createRenderPass();
  createFramebuffers();
  createDescriptors();
  createPipeline();
}

void RasterPass::destroy() {
  mesh_grid_pipeline_.reset();
  mesh_grid_index_buffer_.reset();
  mesh_grid_name_.clear();
  pipeline_.reset();
  descriptor_sets_.reset();
  descriptor_layout_.reset();
  descriptor_pool_.reset();
  framebuffers_.reset();
  render_pass_.reset();
  target_.reset();
}

void RasterPass::update(const RasterPassDesc &desc) { desc_ = desc; }

auto RasterPass::addSource(RenderPassSource source) -> RasterPass & {
  sources_.push_back(source);
  return *this;
}

auto RasterPass::setSources(std::vector<RenderPassSource> sources)
    -> RasterPass & {
  sources_ = std::move(sources);
  return *this;
}

void RasterPass::record() {
  if (!target_ || !render_pass_ || !framebuffers_) {
    VKR_EXEC_ERROR("RasterPass '{}' recorded before create", name());
  }

  syncSelectedMeshGrid();

  RenderPassBeginDesc beginDesc{
      .framebufferIndex = 0,
      .renderArea = {.offset = {0, 0},
                     .extent = {target_->width(), target_->height()}},
      .clearValues = desc_.clearValues};

  executor_.beginPass(*framebuffers_, *render_pass_, beginDesc);
  executor_.setViewportAndScissor({target_->width(), target_->height()});

  if (pipeline_ && pipeline_->valid()) {
    const std::vector<VkDescriptorSet> emptySets{};
    const auto &sets = descriptor_sets_ ? descriptor_sets_->sets() : emptySets;

    executor_.bindPipeline(pipeline_->pipeline(), pipeline_->layout(), sets);
    if (desc_.meshNames.empty()) {
      executor_.drawGeometry();
    } else {
      for (const auto &meshName : desc_.meshNames) {
        auto mesh = scene_.getMesh(meshName);
        if (!mesh || !mesh->isValid()) {
          VKR_EXEC_ERROR("RasterPass '{}' mesh resource not found: {}", name(),
                         meshName);
        }

        const auto vertexBuffer = mesh->vertexBufferBase();
        const auto indexBuffer = mesh->indexBuffer();
        if (!vertexBuffer || !indexBuffer) {
          VKR_EXEC_ERROR("RasterPass '{}' mesh '{}' has invalid buffers",
                         name(), meshName);
        }

        executor_.drawIndexed(vertexBuffer->get(), indexBuffer->get());
      }
    }
    recordSelectedMeshGrid(sets);
  }

  executor_.endPass();
}

auto RasterPass::target() -> OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("RasterPass '{}' target requested before create", name());
  }

  return *target_;
}

auto RasterPass::target() const -> const OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("RasterPass '{}' target requested before create", name());
  }

  return *target_;
}

void RasterPass::createTarget() {
  target_ = std::make_unique<OffscreenTarget>(device_, command_pool_);
  target_->update(desc_.target);
}

void RasterPass::createRenderPass() {
  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  auto renderPassDesc = pipeline::RenderPassDesc::makeOffscreen(
      target_->color().desc().format, target_->depth()
                                          ? target_->depth()->desc().format
                                          : VK_FORMAT_UNDEFINED);

  const auto colorFinalLayout = target_->color().desc().finalLayout;
  if (colorFinalLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
    renderPassDesc.colors[0].finalLayout = colorFinalLayout;
  }

  if (target_->depth()) {
    const auto &depthDesc = target_->depth()->desc();
    if (depthDesc.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
      renderPassDesc.depth.finalLayout = depthDesc.finalLayout;
    }

    if ((depthDesc.usage & VK_IMAGE_USAGE_SAMPLED_BIT) != 0) {
      renderPassDesc.depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }
  }

  render_pass_->update(renderPassDesc);
}

void RasterPass::createFramebuffers() {
  FramebufferDesc framebufferDesc{.width = target_->width(),
                                  .height = target_->height(),
                                  .layers = 1,
                                  .attachments = {target_->attachmentViews()}};

  framebuffers_ = std::make_unique<FramebufferSet>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);
}

void RasterPass::createDescriptors() {
  if (desc_.descriptorBindings.empty() && desc_.inputs.empty()) {
    return;
  }

  if (desc_.inputs.size() != sources_.size()) {
    VKR_EXEC_ERROR("RasterPass '{}' input count mismatch: desc={} sources={}",
                   name(), desc_.inputs.size(), sources_.size());
  }

  std::vector<pipeline::DescriptorBinding> bindings = desc_.descriptorBindings;
  bindings.reserve(desc_.descriptorBindings.size() + desc_.inputs.size());

  for (size_t index = 0; index < desc_.inputs.size(); ++index) {
    const auto &input = desc_.inputs[index];
    bindings.push_back(pipeline::DescriptorBinding{
        .name = "source" + std::to_string(index),
        .layout = {input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                   input.stageFlags}});
  }

  descriptor_pool_ = std::make_unique<pipeline::DescriptorPool>(device_);
  descriptor_pool_->update(descriptorPoolDesc());

  descriptor_layout_ = std::make_unique<pipeline::DescriptorSetLayout>(device_);
  validateUniqueDescriptorBindings(name(), bindings);
  descriptor_layout_->update(
      pipeline::DescriptorSetLayoutDesc{.bindings = bindings});

  descriptor_sets_ = std::make_unique<pipeline::DescriptorSets>(device_);
  descriptor_sets_->update(pipeline::DescriptorSetsDesc{
      .pool = descriptor_pool_->pool(),
      .layout = descriptor_layout_->layout(),
      .setCount = executor_.framesInFlight(),
      .writes = createDescriptorWrites(),
  });
}

void RasterPass::createPipeline() {
  auto pipelineDesc = desc_.graphicsPipeline;
  pipelineDesc.renderPass = render_pass_->renderPass();

  const VkDescriptorSetLayout descriptorSetLayout =
      descriptor_layout_ ? descriptor_layout_->layout() : VK_NULL_HANDLE;
  if (descriptorSetLayout != VK_NULL_HANDLE &&
      pipelineDesc.layout.setLayouts.empty()) {
    pipelineDesc.layout.setLayouts = {descriptorSetLayout};
  }

  if (!pipelineDesc.isValid()) {
    VKR_EXEC_WARN("RasterPass '{}' has no valid graphics pipeline desc",
                  name());
    return;
  }

  pipeline_ = std::make_unique<pipeline::GraphicsPipeline>(device_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR("RasterPass '{}' failed to create graphics pipeline '{}'",
                   name(), pipelineDesc.name);
  }

  auto gridPipelineDesc = pipelineDesc;
  gridPipelineDesc.name = pipelineDesc.name + "-mesh-grid";
  gridPipelineDesc.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  gridPipelineDesc.rasterization =
      pipeline::GraphicsRasterizationDesc::noCull();
  gridPipelineDesc.depthStencil =
      pipeline::GraphicsDepthStencilDesc::readOnly();

  mesh_grid_pipeline_ = std::make_unique<pipeline::GraphicsPipeline>(device_);
  mesh_grid_pipeline_->update(gridPipelineDesc);

  if (!mesh_grid_pipeline_->valid()) {
    VKR_EXEC_ERROR(
        "RasterPass '{}' failed to create mesh grid graphics pipeline '{}'",
        name(), gridPipelineDesc.name);
  }
}

auto RasterPass::createDescriptorWrites() const
    -> std::vector<pipeline::DescriptorSetWriteDesc> {
  std::vector<pipeline::DescriptorSetWriteDesc> writes{};
  const uint32_t frameCount = executor_.framesInFlight();
  writes.reserve(frameCount);

  for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
    writes.push_back(pipeline::DescriptorSetWriteDesc::forSet(frameIndex));
  }

  for (const auto &binding : desc_.descriptorBindings) {
    if (binding.name.empty()) {
      VKR_EXEC_ERROR("Descriptor binding {} has empty resource name",
                     binding.layout.binding);
    }

    switch (binding.layout.descriptorType) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
      auto uniformBuffer = scene_.getUniformBuffer(binding.name);

      if (!uniformBuffer) {
        VKR_EXEC_ERROR("Uniform buffer resource not found: {}", binding.name);
      }

      if (uniformBuffer->frameCount() != frameCount) {
        VKR_EXEC_ERROR("Uniform buffer '{}' frame count mismatch: {} vs {}",
                       binding.name, uniformBuffer->frameCount(), frameCount);
      }

      for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        const auto bufferInfo = uniformBuffer->descriptorInfo(frameIndex);

        writes[frameIndex].buffers.push_back(
            pipeline::DescriptorBufferWriteDesc::one(
                binding.layout.binding, binding.layout.descriptorType,
                bufferInfo));
      }
      break;
    }

    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
      auto texture = scene_.getTexture(binding.name);
      auto cubemap = texture ? nullptr : scene_.getCubemap(binding.name);

      if (!texture && !cubemap) {
        VKR_EXEC_ERROR("Texture or cubemap resource not found: {}",
                       binding.name);
      }

      if (texture && !texture->hasSampler()) {
        VKR_EXEC_ERROR("Texture sampler not found: {}", binding.name);
      }

      if (cubemap && !cubemap->valid()) {
        VKR_EXEC_ERROR("Cubemap resource is invalid: {}", binding.name);
      }

      VkDescriptorImageInfo imageInfo{};
      imageInfo = texture ? VkDescriptorImageInfo{
                                .sampler = texture->sampler(),
                                .imageView = texture->imageView(),
                                .imageLayout = texture->layout(),
                            }
                          : cubemap->descriptorInfo();

      for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        writes[frameIndex].images.push_back(
            pipeline::DescriptorImageWriteDesc::one(
                binding.layout.binding, binding.layout.descriptorType,
                imageInfo));
      }
      break;
    }

    default:
      VKR_EXEC_ERROR("RasterPass '{}' cannot create descriptor writes for "
                     "resource '{}' with descriptor type {}",
                     name(), binding.name,
                     static_cast<int>(binding.layout.descriptorType));
    }
  }

  for (size_t sourceIndex = 0; sourceIndex < sources_.size(); ++sourceIndex) {
    const auto &input = desc_.inputs[sourceIndex];
    const VkDescriptorImageInfo imageInfo =
        sourceImageInfo(name(), sourceIndex, sources_[sourceIndex], input);

    for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
      writes[frameIndex].images.push_back(
          pipeline::DescriptorImageWriteDesc::one(
              input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
              imageInfo));
    }
  }

  return writes;
}

auto RasterPass::descriptorPoolDesc() const -> pipeline::DescriptorPoolDesc {
  auto poolDesc = desc_.descriptorPool;
  if (poolDesc.maxSets != 0) {
    return poolDesc;
  }

  const uint32_t frameCount = executor_.framesInFlight();
  for (const auto &binding : desc_.descriptorBindings) {
    const uint32_t descriptorCount = binding.layout.descriptorCount == 0
                                         ? 1U
                                         : binding.layout.descriptorCount;
    const uint32_t totalCount = frameCount * descriptorCount;

    auto existing =
        std::find_if(poolDesc.poolSizes.begin(), poolDesc.poolSizes.end(),
                     [&binding](const VkDescriptorPoolSize &poolSize) -> bool {
                       return poolSize.type == binding.layout.descriptorType;
                     });

    if (existing != poolDesc.poolSizes.end()) {
      existing->descriptorCount += totalCount;
      continue;
    }

    poolDesc.poolSizes.push_back({binding.layout.descriptorType, totalCount});
  }

  if (!desc_.inputs.empty()) {
    const uint32_t totalCount =
        frameCount * static_cast<uint32_t>(desc_.inputs.size());
    auto existing = std::find_if(
        poolDesc.poolSizes.begin(), poolDesc.poolSizes.end(),
        [](const VkDescriptorPoolSize &poolSize) {
          return poolSize.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        });

    if (existing != poolDesc.poolSizes.end()) {
      existing->descriptorCount += totalCount;
    } else {
      poolDesc.poolSizes.push_back(
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, totalCount});
    }
  }

  poolDesc.maxSets = frameCount;
  return poolDesc;
}

void RasterPass::syncSelectedMeshGrid() {
  const std::string &selectedMesh = scene_.selectedMeshName();

  if (selectedMesh == mesh_grid_name_) {
    return;
  }

  device_.waitIdle();
  mesh_grid_name_ = selectedMesh;
  mesh_grid_index_buffer_.reset();

  if (selectedMesh.empty()) {
    return;
  }

  auto mesh = scene_.getMesh(selectedMesh);
  if (!mesh || !mesh->isValid()) {
    mesh_grid_name_.clear();
    return;
  }

  const auto indexBuffer = mesh->indexBuffer();
  if (!indexBuffer) {
    mesh_grid_name_.clear();
    return;
  }

  const auto indices = indexBuffer->get().indices();
  std::vector<uint16_t> lineIndices;
  lineIndices.reserve(indices.size() * 2);

  std::unordered_set<uint32_t> edges;
  edges.reserve(indices.size());

  auto addEdge = [&](uint16_t a, uint16_t b) -> void {
    const uint16_t lo = std::min(a, b);
    const uint16_t hi = std::max(a, b);
    const uint32_t key =
        (static_cast<uint32_t>(lo) << 16) | static_cast<uint32_t>(hi);

    if (!edges.insert(key).second) {
      return;
    }

    lineIndices.push_back(a);
    lineIndices.push_back(b);
  };

  for (size_t i = 0; i + 2 < indices.size(); i += 3) {
    addEdge(indices[i], indices[i + 1]);
    addEdge(indices[i + 1], indices[i + 2]);
    addEdge(indices[i + 2], indices[i]);
  }

  if (lineIndices.empty()) {
    mesh_grid_name_.clear();
    return;
  }

  mesh_grid_index_buffer_ =
      std::make_unique<scene::IndexBuffer>(device_, command_pool_);
  mesh_grid_index_buffer_->update(lineIndices);
}

void RasterPass::recordSelectedMeshGrid(
    const std::vector<VkDescriptorSet> &sets) {
  if (!mesh_grid_pipeline_ || !mesh_grid_pipeline_->valid() ||
      !mesh_grid_index_buffer_ || mesh_grid_name_.empty()) {
    return;
  }

  if (!desc_.meshNames.empty() &&
      std::find(desc_.meshNames.begin(), desc_.meshNames.end(),
                mesh_grid_name_) == desc_.meshNames.end()) {
    return;
  }

  auto mesh = scene_.getMesh(mesh_grid_name_);
  if (!mesh || !mesh->isValid()) {
    return;
  }

  const auto vertexBuffer = mesh->vertexBufferBase();
  if (!vertexBuffer) {
    return;
  }

  executor_.bindPipeline(mesh_grid_pipeline_->pipeline(),
                         mesh_grid_pipeline_->layout(), sets);
  executor_.drawIndexed(vertexBuffer->get(), *mesh_grid_index_buffer_);
}

} // namespace vkr::exec
