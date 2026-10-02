#pragma once

#include "vkr/exec/graph.hh"
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
struct OverlayPassDesc;
struct RasterPassDesc;
class FeedbackFullscreenPass;
class FullscreenPass;
class OverlayPass;
class PresentCapability;
class RasterPass;
class RenderApplication;
class RenderExecutor;

class RenderGraph final : public Graph {
public:
  RenderGraph(RenderExecutor &executor, const core::Device &device,
              const core::CommandPool &commandPool, scene::Scene &scene);
  ~RenderGraph() override = default;

  void compile() override;
  void destroy() noexcept override;

  auto raster(std::string name, RasterPassDesc desc) -> RasterPass &;
  auto overlay(std::string name, Pass &source, OverlayPassDesc desc)
      -> OverlayPass &;
  auto postProcess(std::string name, Pass &source, FullscreenPassDesc desc)
      -> FullscreenPass &;
  auto fullscreen(std::string name,
                  std::vector<std::reference_wrapper<Pass>> sources,
                  FullscreenPassDesc desc) -> FullscreenPass &;
  auto feedback(std::string name,
                std::vector<std::reference_wrapper<Pass>> sources,
                FeedbackFullscreenPassDesc desc) -> FeedbackFullscreenPass &;
  auto composite(std::string name,
                 std::vector<std::reference_wrapper<Pass>> sources,
                 FullscreenPassDesc desc) -> FullscreenPass &;

  void present(Pass &source);
  void present();

private:
  friend class RenderApplication;

  // dependencies
  RenderExecutor &executor_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  scene::Scene &scene_;

  // state
  std::optional<std::reference_wrapper<Pass>> presentation_source_{};
  std::optional<std::reference_wrapper<PresentCapability>> presenter_{};

  [[nodiscard]] auto presentationSource() -> Pass &;
  void validateSource(const Pass &source) const;
  void addSourceDependency(const Pass &source, const Pass &consumer);
  void addSourceDependencies(
      const std::vector<std::reference_wrapper<Pass>> &sources,
      const Pass &consumer);
  void validate() const override;
};

} // namespace vkr::exec
