#pragma once

#include "vkr/scene/scene.hh"
#include "vkr/ui/components/ui_component.hh"
#include "vkr/ui/selection.hh"
#include <functional>

namespace vkr::ui {

class MeshEditorPanel final : public UiComponent {
public:
  MeshEditorPanel(const scene::Scene &scene, const Selection &selection,
                  std::function<void(Selection)> onSelect);

private:
  void render() override;

  const scene::Scene &scene_;
  const Selection &selection_;
  std::function<void(Selection)> on_select_;
  ImGuiTextFilter filter_{};
};

} // namespace vkr::ui
