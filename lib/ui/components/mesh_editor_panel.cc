#include "vkr/ui/components/mesh_editor_panel.hh"
#include <imgui.h>

namespace vkr::ui {

MeshEditorPanel::MeshEditorPanel(scene::Scene &scene)
    : UiComponent("Mesh Editor"), scene_(scene) {}

void MeshEditorPanel::render() {
  const auto meshNames = scene_.listMeshNames();
  const auto selectedMesh = scene_.selectedMeshName();

  ImGui::SeparatorText("Target");

  const char *preview = selectedMesh.empty() ? "<none>" : selectedMesh.c_str();
  if (ImGui::BeginCombo("Mesh", preview)) {
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

  if (!selectedMesh.empty()) {
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
      scene_.clearSelectedMesh();
    }
  }

  ImGui::SeparatorText("Details");
  const auto currentMesh = scene_.selectedMeshName();
  if (currentMesh.empty()) {
    ImGui::TextDisabled("No mesh selected");
    return;
  }

  ImGui::Text("Name: %s", currentMesh.c_str());

  auto mesh = scene_.getMesh(currentMesh);
  if (!mesh || !mesh->isValid()) {
    ImGui::TextDisabled("State: unavailable");
    return;
  }

  const auto vertexBuffer = mesh->vertexBufferBase();
  const auto indexBuffer = mesh->indexBuffer();
  if (!vertexBuffer || !indexBuffer) {
    ImGui::TextDisabled("State: unavailable");
    return;
  }

  const auto vertexInput = vertexBuffer->get().vertexInputDesc();

  ImGui::Text("Vertices: %zu", vertexBuffer->get().vertexCount());
  ImGui::Text("Indices: %zu", indexBuffer->get().indices().size());
  ImGui::Text("Bindings: %zu", vertexInput.bindings.size());
  ImGui::Text("Attributes: %zu", vertexInput.attributes.size());
}

} // namespace vkr::ui
