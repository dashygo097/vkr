#pragma once

#include "vkr/exec/capability.hh"
#include "vkr/exec/graph.hh"
#include "vkr/ui/components/ui_component.hh"
#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace vkr::ui {

class ExecGraphPanel final : public UiComponent {
public:
  explicit ExecGraphPanel(const exec::Graph &graph);

private:
  void render() override;

  struct PassEntry {
    std::reference_wrapper<const exec::Pass> pass;
    std::optional<
        std::reference_wrapper<const exec::GraphicsPipelineCapability>>
        pipeline;
    std::optional<std::reference_wrapper<const exec::RenderTargetCapability>>
        target;
    bool presents{false};
    std::vector<size_t> dependencies;
    ImVec2 position{};
  };

  struct EdgeEntry {
    size_t producer{};
    size_t consumer{};
    std::array<ImVec2, 6> points{};
    size_t pointCount{};
  };

  void layoutGraph();
  void renderGraph(float height);
  void drawGraph(ImVec2 origin, ImVec2 canvasEnd, float scale,
                 std::optional<size_t> hoveredNode);
  void renderPass(const PassEntry &entry);

  // Cached graph layout, in font-relative coordinates.
  std::vector<PassEntry> passes_{};
  std::vector<EdgeEntry> edges_{};
  ImVec2 graph_min_{};
  ImVec2 graph_max_{};

  // View state.
  size_t selected_pass_{};
  ImVec2 view_offset_{};
  float zoom_{1.0f};
  bool fit_view_{true};
  bool dragging_canvas_{false};
  bool show_details_{false};
};

} // namespace vkr::ui
