#include "vkr/ui/components/mesh_editor_panel.hh"
#include <algorithm>
#include <array>
#include <imgui.h>
#include <string_view>
#include <utility>

namespace vkr::ui {

MeshEditorPanel::MeshEditorPanel(scene::Scene &scene)
    : UiComponent("Mesh Editor"), scene_(scene) {}

void MeshEditorPanel::render() {
  const auto meshNames = scene_.listMeshNames();
  const auto selectedMesh = scene_.selectedMeshName();

  ImGui::SeparatorText("Target");

  const std::string_view preview = selectedMesh.empty()
      ? std::string_view{"Select a mesh"} : std::string_view{selectedMesh};
  const float clearWidth = ImGui::CalcTextSize("Clear").x +
                          ImGui::GetStyle().FramePadding.x * 2.0f;
  ImGui::SetNextItemWidth(std::max(1.0f, ImGui::GetContentRegionAvail().x -
      clearWidth - ImGui::GetStyle().ItemSpacing.x));
  if (ImGui::BeginCombo("##mesh_selection", preview.data())) {
    const bool noneSelected = selectedMesh.empty();
    if (ImGui::Selectable("<none>", noneSelected)) {
      scene_.clearSelectedMesh();
    }

    for (const auto &name : meshNames) {
      const bool selected = selectedMesh == name;
      if (ImGui::Selectable(name.c_str(), selected)) {
        scene_.selectMesh(name);
      }

      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }

    ImGui::EndCombo();
  }

  ImGui::SameLine();
  ImGui::BeginDisabled(selectedMesh.empty());
  if (ImGui::Button("Clear")) {
    scene_.clearSelectedMesh();
  }
  ImGui::EndDisabled();

  ImGui::SeparatorText("Details");
  const auto currentMesh = scene_.selectedMeshName();
  if (currentMesh.empty()) {
    ImGui::TextDisabled("No mesh selected");
    return;
  }

  ImGui::TextWrapped("Name: %s", currentMesh.c_str());

  const auto mesh = scene_.findMesh(currentMesh);
  if (!mesh || !mesh->get().isValid()) {
    ImGui::TextDisabled("State: unavailable");
    return;
  }

  const auto vertexBuffer = mesh->get().vertexBufferBase();
  const auto indexBuffer = mesh->get().indexBuffer();
  if (!vertexBuffer || !indexBuffer) {
    ImGui::TextDisabled("State: unavailable");
    return;
  }

  const auto vertexInput = vertexBuffer->get().vertexInputDesc();

  const std::array<std::pair<std::string_view, size_t>, 4> details{{
      {"Vertices", vertexBuffer->get().vertexCount()},
      {"Indices", indexBuffer->get().indices().size()},
      {"Bindings", vertexInput.bindings.size()},
      {"Attributes", vertexInput.attributes.size()},
  }};
  if (ImGui::BeginTable("##mesh_details", 2, ImGuiTableFlags_RowBg)) {
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed,
                            ImGui::GetFontSize() * 6.0f);
    for (const auto &[label, value] : details) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextDisabled("%s", label.data());
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%zu", value);
    }
    ImGui::EndTable();
  }
}

} // namespace vkr::ui
