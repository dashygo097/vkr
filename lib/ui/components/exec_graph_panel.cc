#include "vkr/ui/components/exec_graph_panel.hh"
#include <imgui.h>

namespace vkr::ui {

ExecGraphPanel::ExecGraphPanel(exec::RenderGraph &graph)
    : UiComponent("Render Graph"), graph_(graph) {}

void ExecGraphPanel::render() {
  const auto passes = graph_.passes();

  ImGui::SeparatorText("Passes");
  ImGui::TextDisabled("%zu total", passes.size());
  ImGui::Spacing();

  for (auto passRef : passes) {
    const auto &pass = passRef.get();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

    const bool open =
        ImGui::TreeNodeEx(pass.name().c_str(), flags, "%s",
                          pass.name().c_str());

    if (!open) {
      continue;
    }

    const auto pipeline = pass.editablePipeline();
    if (pipeline) {
      ImGui::Text("Graphics Pipeline: %s",
                  pipeline->get().desc().name.empty()
                      ? "<unnamed>"
                      : pipeline->get().desc().name.c_str());
    } else {
      ImGui::TextDisabled("Pipeline: none");
    }

    if (ImGui::TreeNodeEx("Reads", ImGuiTreeNodeFlags_SpanAvailWidth)) {
      if (pass.reads().empty()) {
        ImGui::TextDisabled("None");
      } else {
        for (const auto &resource : pass.reads()) {
          ImGui::BulletText("%s", resource.c_str());
        }
      }
      ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Writes", ImGuiTreeNodeFlags_SpanAvailWidth)) {
      if (pass.writes().empty()) {
        ImGui::TextDisabled("None");
      } else {
        for (const auto &resource : pass.writes()) {
          ImGui::BulletText("%s", resource.c_str());
        }
      }
      ImGui::TreePop();
    }

    ImGui::TreePop();
  }
}

} // namespace vkr::ui
