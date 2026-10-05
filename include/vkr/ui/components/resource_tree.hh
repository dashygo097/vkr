#pragma once

#include "vkr/scene/scene.hh"
#include "vkr/ui/components/ui_component.hh"
#include "vkr/ui/selection.hh"
#include <functional>
#include <imgui.h>
#include <string>
#include <string_view>
#include <vector>

namespace vkr::ui {

class ResourceTree final : public UiComponent {
public:
  ResourceTree(const scene::Scene &scene, const Selection &selection,
               std::function<void(Selection)> onSelect);
  ~ResourceTree() = default;

private:
  void render() override;

  const scene::Scene &scene_;
  const Selection &selection_;
  std::function<void(Selection)> on_select_;
  ImGuiTextFilter filter_{};
  bool show_empty_groups_{false};

  void renderCategory(SelectionType type, std::string_view label,
                      std::vector<std::string> names, size_t count);
};

} // namespace vkr::ui
