#pragma once

#include "vkr/exec/graph.hh"
#include "vkr/exec/render/passes/source.hh"
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace vkr::core {
class CommandPool;
class Device;
} // namespace vkr::core

namespace vkr::scene {
class Scene;
} // namespace vkr::scene

namespace vkr::exec {

struct FeedbackFullscreenPassDesc;
struct FullscreenPassDesc;
struct RasterPassDesc;
class CompositePass;
class FeedbackFullscreenPass;
class FullscreenPass;
class PostProcessPass;
class RasterPass;
class RenderApplication;
class RenderExecutor;
class UiPass;

class RenderGraph final : public Graph {
public:
  RenderGraph(RenderExecutor &executor, const core::Device &device,
              const core::CommandPool &commandPool, scene::Scene &scene);
  ~RenderGraph() override = default;

  using Graph::present;

  auto raster(std::string name, RasterPassDesc desc) -> RasterPass &;
  auto postProcess(std::string name, RenderPassSource source,
                   FullscreenPassDesc desc) -> PostProcessPass &;
  auto fullscreen(std::string name, std::vector<RenderPassSource> sources,
                  FullscreenPassDesc desc) -> FullscreenPass &;
  auto feedback(std::string name, std::vector<RenderPassSource> sources,
                FeedbackFullscreenPassDesc desc) -> FeedbackFullscreenPass &;
  auto composite(std::string name, std::vector<RenderPassSource> sources,
                 FullscreenPassDesc desc) -> CompositePass &;

  void present(RasterPass &source);
  void present(FullscreenPass &source);
  void present(FeedbackFullscreenPass &source);

  [[nodiscard]] auto uiPass() -> std::optional<std::reference_wrapper<UiPass>>;
  [[nodiscard]] auto uiPass() const
      -> std::optional<std::reference_wrapper<const UiPass>>;

private:
  friend class RenderApplication;

  // dependencies
  RenderExecutor &executor_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  scene::Scene &scene_;

  // state
  std::optional<RenderPassSource> presentation_source_{};

  void setPresentationSource(RenderPassSource source);
  [[nodiscard]] auto presentationSource() const -> const RenderPassSource &;
  void addSourceDependency(const RenderPassSource &source,
                           const Pass &consumer);
  void addSourceDependencies(const std::vector<RenderPassSource> &sources,
                             const Pass &consumer);
  void validateContract() const override;
};

} // namespace vkr::exec
