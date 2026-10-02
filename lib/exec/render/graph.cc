#include "vkr/exec/render/graph.hh"
#include "vkr/exec/render/passes/composite.hh"
#include "vkr/exec/render/passes/feedback_fullscreen.hh"
#include "vkr/exec/render/passes/fullscreen.hh"
#include "vkr/exec/render/passes/post_process.hh"
#include "vkr/exec/render/passes/raster.hh"
#include "vkr/exec/render/passes/ui.hh"
#include "vkr/logger.hh"
#include <memory>
#include <utility>

namespace vkr::exec {

RenderGraph::RenderGraph(RenderExecutor &executor, const core::Device &device,
                         const core::CommandPool &commandPool,
                         scene::Scene &scene)
    : Graph("Render"), executor_(executor), device_(device),
      command_pool_(commandPool), scene_(scene) {}

auto RenderGraph::raster(std::string name, RasterPassDesc desc)
    -> RasterPass & {
  auto pass =
      std::make_unique<RasterPass>(executor_, device_, command_pool_, scene_);
  pass->setName(std::move(name));
  pass->update(desc);

  auto &result = *pass;
  addPass(std::move(pass));
  return result;
}

auto RenderGraph::postProcess(std::string name, RenderPassSource source,
                              FullscreenPassDesc desc) -> PostProcessPass & {
  auto pass = std::make_unique<PostProcessPass>(
      executor_, device_, command_pool_, source);
  pass->setName(std::move(name));
  pass->update(desc);

  auto &result = *pass;
  addPass(std::move(pass));
  addSourceDependency(source, result);
  return result;
}

auto RenderGraph::fullscreen(std::string name,
                             std::vector<RenderPassSource> sources,
                             FullscreenPassDesc desc) -> FullscreenPass & {
  auto pass = std::make_unique<FullscreenPass>(executor_, device_,
                                               command_pool_, scene_, sources);
  pass->setName(std::move(name));
  pass->update(desc);

  auto &result = *pass;
  addPass(std::move(pass));
  addSourceDependencies(sources, result);
  return result;
}

auto RenderGraph::feedback(std::string name,
                           std::vector<RenderPassSource> sources,
                           FeedbackFullscreenPassDesc desc)
    -> FeedbackFullscreenPass & {
  auto pass = std::make_unique<FeedbackFullscreenPass>(
      executor_, device_, command_pool_, scene_, sources);
  pass->setName(std::move(name));
  pass->update(desc);

  auto &result = *pass;
  addPass(std::move(pass));
  addSourceDependencies(sources, result);
  return result;
}

auto RenderGraph::composite(std::string name,
                            std::vector<RenderPassSource> sources,
                            FullscreenPassDesc desc) -> CompositePass & {
  auto pass = std::make_unique<CompositePass>(executor_, device_,
                                              command_pool_, sources);
  pass->setName(std::move(name));
  pass->update(desc);

  auto &result = *pass;
  addPass(std::move(pass));
  addSourceDependencies(sources, result);
  return result;
}

void RenderGraph::present(RasterPass &source) {
  setPresentationSource(RenderPassSource{source});
}

void RenderGraph::present(FullscreenPass &source) {
  setPresentationSource(RenderPassSource{source});
}

void RenderGraph::present(FeedbackFullscreenPass &source) {
  setPresentationSource(RenderPassSource{source});
}

void RenderGraph::setPresentationSource(RenderPassSource source) {
  if (presentation_source_) {
    VKR_EXEC_ERROR("Render graph presentation source is already set to '{}'",
                   presentation_source_->name());
  }

  if (source.name().empty()) {
    VKR_EXEC_ERROR("Render graph presentation source has no pass name");
  }

  presentation_source_ = std::move(source);
}

auto RenderGraph::presentationSource() const -> const RenderPassSource & {
  if (!presentation_source_) {
    VKR_EXEC_ERROR("Render graph has no presentation source");
  }

  return *presentation_source_;
}

void RenderGraph::addSourceDependency(const RenderPassSource &source,
                                      const Pass &consumer) {
  addDependency(source.name(), consumer.name());
}

void RenderGraph::addSourceDependencies(
    const std::vector<RenderPassSource> &sources, const Pass &consumer) {
  for (const auto &source : sources) {
    addSourceDependency(source, consumer);
  }
}

auto RenderGraph::uiPass() -> std::optional<std::reference_wrapper<UiPass>> {
  for (const auto &pass : passStorage()) {
    try {
      return dynamic_cast<UiPass &>(*pass);
    } catch (const std::bad_cast &) {
    }
  }

  return std::nullopt;
}

auto RenderGraph::uiPass() const
    -> std::optional<std::reference_wrapper<const UiPass>> {
  for (const auto &pass : passStorage()) {
    try {
      return dynamic_cast<const UiPass &>(*pass);
    } catch (const std::bad_cast &) {
    }
  }

  return std::nullopt;
}

void RenderGraph::validateContract() const {
  size_t presenterCount = 0;

  for (const auto &pass : passStorage()) {
    if (pass->capability<PresentCapability>()) {
      presenterCount++;
    }
  }

  if (!presentation_source_) {
    VKR_EXEC_ERROR("Render graph has no presentation source");
  }

  if (presenterCount > 1) {
    VKR_EXEC_ERROR("Render graph has {} PresentPass nodes; only one "
                   "swapchain presenter is allowed",
                   presenterCount);
  }

  if (presenterCount == 0) {
    VKR_EXEC_ERROR("Render graph has no PresentPass");
  }
}

} // namespace vkr::exec
