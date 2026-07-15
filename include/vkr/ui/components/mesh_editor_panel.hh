#pragma once

#include "vkr/resource/manager.hh"
#include "vkr/ui/components/ui_component.hh"

namespace vkr::ui {

class MeshEditorPanel final : public UiComponent {
public:
  explicit MeshEditorPanel(resource::ResourceManager &resourceManager);

private:
  void render() override;

  resource::ResourceManager &resource_manager_;
};

} // namespace vkr::ui
