#include "vkr/exec/render/passes/fullscreen.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <string_view>

namespace vkr::exec {
namespace {

auto imageLayoutForColor(const pipeline::ColorAttachment &color)
    -> VkImageLayout {
  return color.desc().finalLayout == VK_IMAGE_LAYOUT_UNDEFINED
             ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
             : color.desc().finalLayout;
}

auto sourceImageInfo(std::string_view passName, size_t sourceIndex,
                     const pipeline::OffscreenTarget &source,
                     const RenderPassInputDesc &input)
    -> VkDescriptorImageInfo {
  VkDescriptorImageInfo imageInfo{};

  switch (input.kind) {
  case RenderPassInputKind::Color: {
    if (!source.hasColor()) {
      VKR_EXEC_ERROR("FullscreenPass '{}' source {} has no color attachment",
                     std::string(passName), sourceIndex);
    }

    const auto &color = source.color();
    if (!color.hasSampler()) {
      VKR_EXEC_ERROR("FullscreenPass '{}' source {} color has no sampler",
                     std::string(passName), sourceIndex);
    }

    imageInfo.imageLayout = imageLayoutForColor(color);
    imageInfo.imageView = color.imageView();
    imageInfo.sampler = color.sampler();
    break;
  }

  case RenderPassInputKind::Depth: {
    const auto *depth = source.depth();
    if (depth == nullptr) {
      VKR_EXEC_ERROR("FullscreenPass '{}' source {} has no depth attachment",
                     std::string(passName), sourceIndex);
    }

    if (!depth->hasSampler()) {
      VKR_EXEC_ERROR("FullscreenPass '{}' source {} depth has no sampler",
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

} // namespace

FullscreenPass::FullscreenPass(
    RenderExecutor &executor, const core::Device &device,
    std::vector<std::reference_wrapper<Pass>> sources)
    : executor_(executor), device_(device), sources_(std::move(sources)),
      descriptor_pool_(device), descriptor_layout_(device),
      descriptor_sets_(device, descriptor_pool_, descriptor_layout_) {}

FullscreenPass::~FullscreenPass() { destroy(); }

void FullscreenPass::create() {
  validate(desc_);
  destroy();

  createTarget();
  createRenderPass();
  createFramebuffers();
  createDescriptors();
  createPipeline();
}

void FullscreenPass::destroy() noexcept {
  pipeline_.reset();
  descriptor_sets_.destroy();
  descriptor_layout_.destroy();
  descriptor_pool_.destroy();
  framebuffers_.reset();
  render_pass_.reset();
  target_.reset();
}

void FullscreenPass::update(const FullscreenPassDesc &desc) {
  ensureConfigurable();
  if (target_ || render_pass_ || pipeline_ || descriptor_layout_.valid()) {
    VKR_EXEC_ERROR("FullscreenPass '{}' must be destroyed before updating its "
                   "configuration",
                   name());
  }
  validate(desc);
  auto nextDesc = desc;
  desc_ = std::move(nextDesc);
}

void FullscreenPass::record() {
  if (!target_ || !render_pass_ || !framebuffers_) {
    VKR_EXEC_ERROR("FullscreenPass '{}' recorded before create", name());
  }

  executor_.beginProfileScope(name());
  executor_.beginPass(*render_pass_, *framebuffers_, desc_.clearValues);
  executor_.setViewportAndScissor({target_->width(), target_->height()});

  if (pipeline_ && pipeline_->valid()) {
    descriptor_sets_.write(executor_.frameIndex());
    if (descriptor_sets_.valid()) {
      executor_.bindPipeline(*pipeline_, descriptor_sets_);
    } else {
      executor_.bindPipeline(*pipeline_);
    }
    executor_.drawFullscreenTriangle();
  }

  executor_.endPass();
  executor_.endProfileScope();
}

auto FullscreenPass::addSource(Pass &source) -> FullscreenPass & {
  ensureConfigurable();
  sources_.emplace_back(source);
  return *this;
}

auto FullscreenPass::setSources(
    std::vector<std::reference_wrapper<Pass>> sources) -> FullscreenPass & {
  ensureConfigurable();
  sources_ = std::move(sources);
  return *this;
}

auto FullscreenPass::target() -> pipeline::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FullscreenPass '{}' target requested before create",
                   name());
  }

  return *target_;
}

auto FullscreenPass::target() const -> const pipeline::OffscreenTarget & {
  if (!target_) {
    VKR_EXEC_ERROR("FullscreenPass '{}' target requested before create",
                   name());
  }

  return *target_;
}

void FullscreenPass::createTarget() {
  target_ = std::make_unique<pipeline::OffscreenTarget>(device_);
  target_->update(desc_.target);
}

void FullscreenPass::createRenderPass() {
  render_pass_ = std::make_unique<pipeline::RenderPass>(device_);
  pipeline::RenderPassDesc renderPassDesc{};

  if (target_->hasColor()) {
    renderPassDesc = pipeline::RenderPassDesc::makeOffscreen(
        target_->color().desc().format, target_->depth()
                                            ? target_->depth()->desc().format
                                            : VK_FORMAT_UNDEFINED);

    const auto &colorDesc = target_->color().desc();
    if (colorDesc.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
      renderPassDesc.colors[0].finalLayout = colorDesc.finalLayout;
    } else if ((colorDesc.usage & (VK_IMAGE_USAGE_SAMPLED_BIT |
                                   VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)) == 0) {
      renderPassDesc.colors[0].finalLayout =
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
  } else if (target_->depth()) {
    renderPassDesc = pipeline::RenderPassDesc::makeDepthOnly(
        target_->depth()->desc().format);
  } else {
    VKR_EXEC_ERROR("FullscreenPass '{}' target has no attachments", name());
  }

  if (target_->depth()) {
    const auto &depthDesc = target_->depth()->desc();
    if (depthDesc.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
      renderPassDesc.depth.finalLayout = depthDesc.finalLayout;
    }

    renderPassDesc.depth.storeOp =
        (depthDesc.usage & VK_IMAGE_USAGE_SAMPLED_BIT) != 0
            ? VK_ATTACHMENT_STORE_OP_STORE
            : depthDesc.storeOp;
  }

  render_pass_->update(renderPassDesc);
}

void FullscreenPass::createFramebuffers() {
  auto framebufferDesc = FramebuffersDesc::single(
      target_->width(), target_->height(), target_->attachmentViews());

  framebuffers_ = std::make_unique<Framebuffers>(device_, *render_pass_);
  framebuffers_->update(framebufferDesc);
}

void FullscreenPass::validate(const FullscreenPassDesc &desc) const {
  if (!desc.target.isValid()) {
    VKR_EXEC_ERROR("FullscreenPass '{}' has an invalid target descriptor",
                   name());
  }
  if (!desc.pipeline.isValid()) {
    VKR_EXEC_ERROR("FullscreenPass '{}' has an invalid pipeline descriptor",
                   name());
  }
  pipeline::DescriptorSetLayoutDesc layoutDesc{.bindings =
                                                   desc.descriptorBindings};
  for (const auto &input : desc.inputs) {
    layoutDesc.bindings.push_back(
        {.layout = {input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    input.stageFlags}});
  }
  if (!layoutDesc.isValid()) {
    VKR_EXEC_ERROR("FullscreenPass '{}' requires unique bindings with nonzero "
                   "counts/stages",
                   name());
  }
  for (const auto &binding : layoutDesc.bindings) {
    const auto &layout = binding.layout;
    if (layout.descriptorCount != 1 ||
        (layout.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
         layout.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
         layout.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) {
      VKR_EXEC_ERROR(
          "FullscreenPass '{}' has an unsupported descriptor binding {}",
          name(), layout.binding);
    }
  }
}

void FullscreenPass::createDescriptors() {
  const auto inputs = resolvedInputs();
  if (inputs.empty() && desc_.descriptorBindings.empty()) {
    descriptor_sets_.update({.setCount = 0});
    return;
  }

  std::vector<pipeline::DescriptorBinding> bindings = desc_.descriptorBindings;
  bindings.reserve(desc_.descriptorBindings.size() + inputs.size());

  for (const auto &input : inputs) {
    bindings.push_back(pipeline::DescriptorBinding{
        .layout = {input.binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                   input.stageFlags}});
  }

  const pipeline::DescriptorSetLayoutDesc layoutDesc{.bindings = bindings};
  descriptor_pool_.update(pipeline::DescriptorPoolDesc::sets(
      layoutDesc, executor_.framesInFlight()));
  descriptor_layout_.update(layoutDesc);

  descriptor_sets_.update({.setCount = executor_.framesInFlight()});
  descriptor_sets_.write(createDescriptorWrites(inputs));
  for (uint32_t index = 0; index < descriptor_sets_.count(); ++index) {
    descriptor_sets_.write(index);
  }
}

void FullscreenPass::createPipeline() {
  auto pipelineDesc = desc_.pipeline;

  const VkDescriptorSetLayout descriptorSetLayout =
      descriptor_layout_.valid() ? descriptor_layout_.layout() : VK_NULL_HANDLE;
  if (descriptorSetLayout != VK_NULL_HANDLE &&
      pipelineDesc.layout.setLayouts.empty()) {
    pipelineDesc.layout.setLayouts = {descriptorSetLayout};
  }

  pipeline_ =
      std::make_unique<pipeline::GraphicsPipeline>(device_, *render_pass_);
  pipeline_->update(pipelineDesc);

  if (!pipeline_->valid()) {
    VKR_EXEC_ERROR(
        "FullscreenPass '{}' failed to create graphics pipeline '{}'", name(),
        pipelineDesc.name);
  }
}

auto FullscreenPass::resolvedInputs() const
    -> std::vector<RenderPassInputDesc> {
  if (desc_.inputs.empty()) {
    std::vector<RenderPassInputDesc> inputs{};
    inputs.reserve(sources_.size());

    uint32_t firstBinding = 0;
    for (const auto &binding : desc_.descriptorBindings) {
      firstBinding = std::max(firstBinding, binding.layout.binding + 1U);
    }

    for (uint32_t index = 0; index < sources_.size(); ++index) {
      inputs.push_back(RenderPassInputDesc{.binding = firstBinding + index});
    }

    return inputs;
  }

  if (desc_.inputs.size() != sources_.size()) {
    VKR_EXEC_ERROR(
        "FullscreenPass '{}' input count mismatch: desc={} sources={}", name(),
        desc_.inputs.size(), sources_.size());
  }

  return desc_.inputs;
}

auto FullscreenPass::createDescriptorWrites(
    const std::vector<RenderPassInputDesc> &inputs)
    -> std::vector<pipeline::DescriptorSetWrite> {
  std::vector<pipeline::DescriptorSetWrite> writes{};
  const uint32_t frameCount = executor_.framesInFlight();
  writes.reserve(frameCount);

  for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
    writes.push_back(pipeline::DescriptorSetWrite::forSet(frameIndex));
  }

  for (size_t index = 0; index < sources_.size(); ++index) {
    const auto source =
        sources_[index].get().capability<RenderTargetCapability>();
    if (!source) {
      VKR_EXEC_ERROR("FullscreenPass '{}' source '{}' has no render target",
                     name(), sources_[index].get().name());
    }
    for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
      const VkDescriptorImageInfo imageInfo = sourceImageInfo(
          name(), index, source->get().target(frameIndex), inputs[index]);

      auto &write = writes[frameIndex];
      write.images.push_back(pipeline::DescriptorImageWrite::one(
          inputs[index].binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          imageInfo));
    }
  }

  return writes;
}

} // namespace vkr::exec
