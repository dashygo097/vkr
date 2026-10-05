#pragma once

#include "vkr/exec/graph.hh"
#include "vkr/ui/components/ui_component.hh"
#include "vkr/ui/selection.hh"
#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace vkr::ui {

class ExecGraphPanel final : public UiComponent {
public:
  ExecGraphPanel(const exec::Graph &graph, const Selection &selection,
                 std::function<void(Selection)> onSelect);

private:
  void render() override;

  struct PassEntry {
    std::reference_wrapper<const exec::Pass> pass;
    bool pipeline{false};
    bool target{false};
    bool presents{false};
    std::vector<size_t> dependencies;
    size_t consumerCount{};
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

  const Selection &selection_;
  std::function<void(Selection)> on_select_;

  // Cached graph layout, in font-relative coordinates.
  std::vector<PassEntry> passes_{};
  std::vector<EdgeEntry> edges_{};
  ImVec2 graph_min_{};
  ImVec2 graph_max_{};

  // View state.
  ImVec2 view_offset_{};
  float zoom_{1.0f};
  bool view_initialized_{false};
  bool fit_view_{false};
  bool dragging_canvas_{false};
};

} // namespace vkr::ui
