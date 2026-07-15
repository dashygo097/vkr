#pragma once

#include "vkr/render/graph.hh"
#include "vkr/ui/components/ui_component.hh"

namespace vkr::ui {

class RenderGraphPanel final : public UiComponent {
public:
  explicit RenderGraphPanel(render::RenderGraph &graph);

private:
  void render() override;

  render::RenderGraph &graph_;
};

} // namespace vkr::ui
