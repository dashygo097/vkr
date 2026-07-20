#pragma once

#include "vkr/exec/render/graph.hh"
#include "vkr/ui/components/ui_component.hh"

namespace vkr::ui {

class ExecGraphPanel final : public UiComponent {
public:
  explicit ExecGraphPanel(exec::RenderGraph &graph);

private:
  void render() override;

  exec::RenderGraph &graph_;
};

} // namespace vkr::ui
