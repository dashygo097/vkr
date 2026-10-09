#pragma once

#include "vkr/core/command/buffers.hh"
#include "vkr/exec/capability.hh"
#include "vkr/exec/graph.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/scene/scene.hh"
#include "vkr/ui/components/ui_component.hh"
#include "vkr/ui/selection.hh"
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace vkr::ui {

class TexturePreview;

class InspectorPanel final : public UiComponent {
public:
  InspectorPanel(const core::Device &device,
                 const pipeline::RenderPass &renderPass,
                 const core::CommandBuffers &commandBuffers,
                 const scene::Scene &scene, const exec::Graph &graph,
                 const Selection &selection,
                 std::function<void(Selection)> onSelect);
  ~InspectorPanel() override;

  void prepare(uint32_t frameIndex);

private:
  void render() override;
  void renderResource();
  void renderPass();

  const scene::Scene &scene_;
  const exec::Graph &graph_;
  const Selection &selection_;
  std::function<void(Selection)> on_select_;
  std::unique_ptr<TexturePreview> texture_preview_;

  // Resolved once per pass selection, not once per UI frame.
  struct PassEntry {
    std::reference_wrapper<const exec::Pass> pass;
    std::optional<
        std::reference_wrapper<const exec::GraphicsPipelineCapability>>
        pipeline;
    std::optional<std::reference_wrapper<const exec::RenderTargetCapability>>
        target;
    bool presents{false};
    std::vector<std::reference_wrapper<const exec::Pass>> dependencies;
  };
  std::optional<PassEntry> inspected_pass_{};
};

} // namespace vkr::ui
