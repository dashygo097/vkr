#include "vkr/ui/components/mesh_editor_panel.hh"
#include <imgui.h>

namespace vkr::ui {

MeshEditorPanel::MeshEditorPanel(resource::ResourceManager &resourceManager)
    : UiComponent("Mesh Editor"), resource_manager_(resourceManager) {}

void MeshEditorPanel::render() {
  const auto meshNames = resource_manager_.listMeshNames();
  const auto selectedMesh = resource_manager_.selectedMeshName();

  ImGui::SeparatorText("Target");

  const char *preview = selectedMesh.empty() ? "<none>" : selectedMesh.c_str();
  if (ImGui::BeginCombo("Mesh", preview)) {
    const bool noneSelected = selectedMesh.empty();
    if (ImGui::Selectable("<none>", noneSelected)) {
      resource_manager_.clearSelectedMesh();
    }

    for (const auto &name : meshNames) {
      const bool selected = selectedMesh == name;
      if (ImGui::Selectable(name.c_str(), selected)) {
        resource_manager_.selectMesh(name);
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
      resource_manager_.clearSelectedMesh();
    }
  }

  ImGui::SeparatorText("Details");
  const auto currentMesh = resource_manager_.selectedMeshName();
  if (currentMesh.empty()) {
    ImGui::TextDisabled("No mesh selected");
    return;
  }

  ImGui::Text("Name: %s", currentMesh.c_str());

  auto mesh = resource_manager_.getMesh(currentMesh);
  if (!mesh || !mesh->isValid()) {
    ImGui::TextDisabled("State: unavailable");
    return;
  }

  const auto *vertexBuffer = mesh->vertexBufferBase();
  const auto *indexBuffer = mesh->indexBuffer();
  const auto vertexInput = vertexBuffer->vertexInputDesc();

  ImGui::Text("Vertices: %zu", vertexBuffer->vertexCount());
  ImGui::Text("Indices: %zu", indexBuffer->indices().size());
  ImGui::Text("Bindings: %zu", vertexInput.bindings.size());
  ImGui::Text("Attributes: %zu", vertexInput.attributes.size());
}

} // namespace vkr::ui
