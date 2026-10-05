#include "vkr/ui/components/mesh_editor_panel.hh"
#include <imgui.h>
#include <utility>

namespace vkr::ui {

MeshEditorPanel::MeshEditorPanel(const scene::Scene &scene,
                                 const Selection &selection,
                                 std::function<void(Selection)> onSelect)
    : UiComponent("Mesh Editor"), scene_(scene), selection_(selection),
      on_select_(std::move(onSelect)) {}

void MeshEditorPanel::render() {
  const auto meshNames = scene_.listMeshNames();
  ImGui::SetNextItemWidth(-1.0f);
  if (ImGui::InputTextWithHint("##mesh_filter", "Filter meshes...",
                              filter_.InputBuf, sizeof(filter_.InputBuf))) {
    filter_.Build();
  }

  ImGui::BeginDisabled(selection_.type != SelectionType::Mesh);
  if (ImGui::Button("Clear selection")) {
    on_select_({});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::TextDisabled("%zu meshes", meshNames.size());

  if (meshNames.empty()) {
    ImGui::TextDisabled("No meshes");
    return;
  }

  if (ImGui::BeginChild("##mesh_list", ImVec2(0.0f, 0.0f))) {
    for (const auto &name : meshNames) {
      if (!filter_.PassFilter(name.c_str())) {
        continue;
      }
      const bool selected = selection_.type == SelectionType::Mesh &&
                            selection_.name == name;
      if (ImGui::Selectable(name.c_str(), selected)) {
        on_select_({SelectionType::Mesh, name});
      }
    }
  }
  ImGui::EndChild();
}

} // namespace vkr::ui
