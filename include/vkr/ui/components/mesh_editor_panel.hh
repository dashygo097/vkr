#pragma once

#include "vkr/scene/scene.hh"
#include "vkr/ui/components/ui_component.hh"

namespace vkr::ui {

class MeshEditorPanel final : public UiComponent {
public:
  explicit MeshEditorPanel(scene::Scene &scene);

private:
  void render() override;

  scene::Scene &scene_;
};

} // namespace vkr::ui
