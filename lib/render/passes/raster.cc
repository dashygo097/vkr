#include "vkr/render/passes/raster.hh"
#include "vkr/logger.hh"
#include "vkr/render/renderer.hh"
#include <algorithm>
#include <unordered_set>

namespace vkr::render {

RasterPass::RasterPass(Renderer &renderer, const core::Device &device,
                       const core::CommandPool &commandPool,
                       resource::ResourceManager &resourceManager)
    : renderer_(renderer), device_(device), command_pool_(commandPool),
      resource_manager_(resourceManager) {}

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

void RasterPass::record() {
  if (!target_ || !render_pass_ || !framebuffers_) {
    VKR_RENDER_ERROR("RasterPass '{}' recorded before create", name());
  }

  syncSelectedMeshGrid();

  RenderPassBeginDesc beginDesc{
      .framebufferIndex = 0,
      .renderArea = {.offset = {0, 0},
                     .extent = {target_->width(), target_->height()}},
      .clearValues = desc_.clearValues};

  renderer_.beginPass(*framebuffers_, *render_pass_, beginDesc);
  renderer_.setViewportAndScissor({target_->width(), target_->height()});

  if (pipeline_ && pipeline_->valid()) {
    const std::vector<VkDescriptorSet> emptySets{};
    const auto &sets = descriptor_sets_ ? descriptor_sets_->sets() : emptySets;

    renderer_.bindPipeline(pipeline_->pipeline(), pipeline_->layout(), sets);
    renderer_.drawGeometry();
    recordSelectedMeshGrid(sets);
  }

  renderer_.endPass();
}

auto RasterPass::target() -> resource::OffscreenTarget & {
  if (!target_) {
    VKR_RENDER_ERROR("RasterPass '{}' target requested before create", name());
  }

  return *target_;
}

auto RasterPass::target() const -> const resource::OffscreenTarget & {
  if (!target_) {
    VKR_RENDER_ERROR("RasterPass '{}' target requested before create", name());
  }

  return *target_;
}

void RasterPass::createTarget() {
  target_ = std::make_unique<resource::OffscreenTarget>(device_, command_pool_);
  target_->update(desc_.target);
}

void RasterPass::createRenderPass() {
  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  render_pass_->update(pipeline::RenderPassDesc::makeOffscreen(
      target_->color().desc().format, target_->depth()
                                          ? target_->depth()->desc().format
                                          : VK_FORMAT_UNDEFINED));
}

void RasterPass::createFramebuffers() {
  resource::FramebufferDesc framebufferDesc{
      .width = target_->width(),
      .height = target_->height(),
      .layers = 1,
      .attachments = {target_->attachmentViews()}};

  framebuffers_ =
      std::make_unique<resource::FramebufferSet>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);
}

void RasterPass::createDescriptors() {
  if (desc_.descriptorBindings.empty()) {
    return;
  }

  descriptor_pool_ = std::make_unique<pipeline::DescriptorPool>(device_);
  descriptor_pool_->update(desc_.descriptorPool);

  descriptor_layout_ = std::make_unique<pipeline::DescriptorSetLayout>(device_);
  descriptor_layout_->update(
      pipeline::DescriptorSetLayoutDesc{.bindings = desc_.descriptorBindings});

  descriptor_sets_ = std::make_unique<pipeline::DescriptorSets>(device_);
  descriptor_sets_->update(pipeline::DescriptorSetsDesc{
      .pool = descriptor_pool_->pool(),
      .layout = descriptor_layout_->layout(),
      .setCount = core::MAX_FRAMES_IN_FLIGHT,
      .writes = createDescriptorWrites(),
  });
}

void RasterPass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;
  pipelineDesc.renderPass = render_pass_->renderPass();

  const VkDescriptorSetLayout descriptorSetLayout =
      descriptor_layout_ ? descriptor_layout_->layout() : VK_NULL_HANDLE;
  if (descriptorSetLayout != VK_NULL_HANDLE &&
      pipelineDesc.layout.setLayouts.empty()) {
    pipelineDesc.layout.setLayouts = {descriptorSetLayout};
  }

  if (!pipelineDesc.isValid()) {
    VKR_RENDER_WARN("RasterPass '{}' has no valid graphics pipeline desc",
                    name());
    return;
  }

  pipeline_ = std::make_unique<pipeline::GraphicsPipeline>(device_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_RENDER_ERROR("RasterPass '{}' failed to create graphics pipeline '{}'",
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
    VKR_RENDER_ERROR(
        "RasterPass '{}' failed to create mesh grid graphics pipeline '{}'",
        name(), gridPipelineDesc.name);
  }
}

auto RasterPass::createDescriptorWrites() const
    -> std::vector<pipeline::DescriptorSetWriteDesc> {
  std::vector<pipeline::DescriptorSetWriteDesc> writes{};
  writes.reserve(core::MAX_FRAMES_IN_FLIGHT);

  for (uint32_t frameIndex = 0; frameIndex < core::MAX_FRAMES_IN_FLIGHT;
       ++frameIndex) {
    writes.push_back(pipeline::DescriptorSetWriteDesc::forSet(frameIndex));
  }

  for (const auto &binding : desc_.descriptorBindings) {
    if (binding.name.empty()) {
      VKR_RENDER_ERROR("Descriptor binding {} has empty resource name",
                       binding.layout.binding);
    }

    switch (binding.layout.descriptorType) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
      auto uniformBuffer = resource_manager_.getUniformBuffer(binding.name);

      if (!uniformBuffer) {
        VKR_RENDER_ERROR("Uniform buffer resource not found: {}", binding.name);
      }

      const auto &buffers = uniformBuffer->buffers();
      if (buffers.size() != core::MAX_FRAMES_IN_FLIGHT) {
        VKR_RENDER_ERROR("Uniform buffer '{}' frame count mismatch: {} vs {}",
                         binding.name, buffers.size(),
                         core::MAX_FRAMES_IN_FLIGHT);
      }

      for (uint32_t frameIndex = 0; frameIndex < core::MAX_FRAMES_IN_FLIGHT;
           ++frameIndex) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffers[frameIndex];
        bufferInfo.offset = 0;
        bufferInfo.range = uniformBuffer->bufferSize();

        writes[frameIndex].buffers.push_back(
            pipeline::DescriptorBufferWriteDesc::one(
                binding.layout.binding, binding.layout.descriptorType,
                bufferInfo));
      }
      break;
    }

    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
      auto texture = resource_manager_.getTexture(binding.name);

      if (!texture) {
        VKR_RENDER_ERROR("Texture resource not found: {}", binding.name);
      }

      if (!texture->hasSampler()) {
        VKR_RENDER_ERROR("Texture sampler not found: {}", binding.name);
      }

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = texture->layout();
      imageInfo.imageView = texture->imageView();
      imageInfo.sampler = texture->sampler();

      for (uint32_t frameIndex = 0; frameIndex < core::MAX_FRAMES_IN_FLIGHT;
           ++frameIndex) {
        writes[frameIndex].images.push_back(
            pipeline::DescriptorImageWriteDesc::one(
                binding.layout.binding, binding.layout.descriptorType,
                imageInfo));
      }
      break;
    }

    default:
      VKR_RENDER_ERROR("RasterPass '{}' cannot create descriptor writes for "
                       "resource '{}' with descriptor type {}",
                       name(), binding.name,
                       static_cast<int>(binding.layout.descriptorType));
    }
  }

  return writes;
}

void RasterPass::syncSelectedMeshGrid() {
  const std::string &selectedMesh = resource_manager_.selectedMeshName();

  if (selectedMesh == mesh_grid_name_) {
    return;
  }

  device_.waitIdle();
  mesh_grid_name_ = selectedMesh;
  mesh_grid_index_buffer_.reset();

  if (selectedMesh.empty()) {
    return;
  }

  auto mesh = resource_manager_.getMesh(selectedMesh);
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

  auto addEdge = [&](uint16_t a, uint16_t b) {
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
      std::make_unique<resource::IndexBuffer>(device_, command_pool_);
  mesh_grid_index_buffer_->update(lineIndices);
}

void RasterPass::recordSelectedMeshGrid(
    const std::vector<VkDescriptorSet> &sets) {
  if (!mesh_grid_pipeline_ || !mesh_grid_pipeline_->valid() ||
      !mesh_grid_index_buffer_ || mesh_grid_name_.empty()) {
    return;
  }

  auto mesh = resource_manager_.getMesh(mesh_grid_name_);
  if (!mesh || !mesh->isValid()) {
    return;
  }

  const auto vertexBuffer = mesh->vertexBufferBase();
  if (!vertexBuffer) {
    return;
  }

  renderer_.bindPipeline(mesh_grid_pipeline_->pipeline(),
                         mesh_grid_pipeline_->layout(), sets);
  renderer_.drawIndexed(vertexBuffer->get(), *mesh_grid_index_buffer_);
}

} // namespace vkr::render
