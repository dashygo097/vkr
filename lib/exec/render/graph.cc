#include "vkr/exec/render/graph.hh"
#include "vkr/exec/capability.hh"
#include "vkr/exec/render/passes/feedback_fullscreen.hh"
#include "vkr/exec/render/passes/fullscreen.hh"
#include "vkr/exec/render/passes/overlay.hh"
#include "vkr/exec/render/passes/raster.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <memory>
#include <utility>

namespace vkr::exec {

RenderGraph::RenderGraph(RenderExecutor &executor, const core::Device &device,
                         const core::CommandPool &commandPool,
                         scene::Scene &scene)
    : Graph("Render"), executor_(executor), device_(device),
      command_pool_(commandPool), scene_(scene) {}

void RenderGraph::compile() {
  Graph::compile();

  for (const auto &pass : passStorage()) {
    const auto presenter = pass->capability<PresentCapability>();
    if (presenter) {
      presenter_ = presenter->get();
      break;
    }
  }
}

void RenderGraph::destroy() noexcept {
  presenter_.reset();
  Graph::destroy();
}

auto RenderGraph::raster(std::string name, RasterPassDesc desc)
    -> RasterPass & {
  auto pass = std::make_unique<RasterPass>(executor_, device_, scene_);
  pass->setName(std::move(name));
  pass->update(desc);

  auto &result = *pass;
  addPass(std::move(pass));
  return result;
}

auto RenderGraph::overlay(std::string name, Pass &source, OverlayPassDesc desc)
    -> OverlayPass & {
  ensureBuilding();
  validateSource(source);
  auto pass = std::make_unique<OverlayPass>(executor_, device_, command_pool_,
                                            scene_, source);
  pass->setName(std::move(name));
  pass->update(desc);
  auto &result = *pass;
  addPass(std::move(pass));
  addSourceDependency(source, result);
  return result;
}

auto RenderGraph::postProcess(std::string name, Pass &source,
                              FullscreenPassDesc desc) -> FullscreenPass & {
  return fullscreen(std::move(name), {source}, std::move(desc));
}

auto RenderGraph::fullscreen(std::string name,
                             std::vector<std::reference_wrapper<Pass>> sources,
                             FullscreenPassDesc desc) -> FullscreenPass & {
  ensureBuilding();
  for (const auto &source : sources) {
    validateSource(source.get());
  }
  auto pass = std::make_unique<FullscreenPass>(executor_, device_, sources);
  pass->setName(std::move(name));
  pass->update(desc);

  auto &result = *pass;
  addPass(std::move(pass));
  addSourceDependencies(sources, result);
  return result;
}

auto RenderGraph::feedback(std::string name,
                           std::vector<std::reference_wrapper<Pass>> sources,
                           FeedbackFullscreenPassDesc desc)
    -> FeedbackFullscreenPass & {
  ensureBuilding();
  for (const auto &source : sources) {
    validateSource(source.get());
  }
  auto pass =
      std::make_unique<FeedbackFullscreenPass>(executor_, device_, sources);
  pass->setName(std::move(name));
  pass->update(desc);

  auto &result = *pass;
  addPass(std::move(pass));
  addSourceDependencies(sources, result);
  return result;
}

auto RenderGraph::composite(std::string name,
                            std::vector<std::reference_wrapper<Pass>> sources,
                            FullscreenPassDesc desc) -> FullscreenPass & {
  return fullscreen(std::move(name), std::move(sources), std::move(desc));
}

void RenderGraph::present(Pass &source) {
  ensureBuilding();
  validateSource(source);
  if (presentation_source_) {
    VKR_EXEC_ERROR("Render graph presentation source is already set to '{}'",
                   presentation_source_->get().name());
  }

  if (source.name().empty()) {
    VKR_EXEC_ERROR("Render graph presentation source has no pass name");
  }

  presentation_source_ = source;
}

void RenderGraph::present() {
  ensureCreated();
  if (!presenter_) {
    VKR_EXEC_ERROR("Render graph has no presentation endpoint");
  }

  presenter_->get().present();
}

auto RenderGraph::presentationSource() -> Pass & {
  if (!presentation_source_) {
    VKR_EXEC_ERROR("Render graph has no presentation source");
  }

  return presentation_source_->get();
}

void RenderGraph::validateSource(const Pass &source) const {
  if (!source.capability<RenderTargetCapability>()) {
    VKR_EXEC_ERROR("Source pass '{}' has no render target", source.name());
  }

  const auto &storage = passStorage();
  const bool owned = std::any_of(
      storage.begin(), storage.end(), [&source](const auto &pass) -> bool {
        return std::addressof(*pass) == std::addressof(source);
      });
  if (!owned) {
    VKR_EXEC_ERROR("Source pass '{}' does not belong to this graph",
                   source.name());
  }
}

void RenderGraph::addSourceDependency(const Pass &source,
                                      const Pass &consumer) {
  addDependency(source.name(), consumer.name());
}

void RenderGraph::addSourceDependencies(
    const std::vector<std::reference_wrapper<Pass>> &sources,
    const Pass &consumer) {
  for (const auto &source : sources) {
    addSourceDependency(source.get(), consumer);
  }
}

void RenderGraph::validate() const {
  if (!presentation_source_) {
    VKR_EXEC_ERROR("Render graph has no presentation source");
  }

  size_t presenterCount = 0;
  for (const auto &pass : passStorage()) {
    if (pass->capability<PresentCapability>()) {
      ++presenterCount;
    }
  }

  if (presenterCount != 1) {
    VKR_EXEC_ERROR("Render graph requires exactly one presentation endpoint; "
                   "found {}",
                   presenterCount);
  }
}

} // namespace vkr::exec
