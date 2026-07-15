#include "vkr/ui/components/render_graph_panel.hh"
#include <imgui.h>

namespace vkr::ui {

RenderGraphPanel::RenderGraphPanel(render::RenderGraph &graph)
    : UiComponent("Render Graph"), graph_(graph) {}

void RenderGraphPanel::render() {
  const auto passes = graph_.passes();

  ImGui::SeparatorText("Passes");
  ImGui::TextDisabled("%zu total", passes.size());
  ImGui::Spacing();

  for (const auto *pass : passes) {
    if (pass == nullptr) {
      continue;
    }

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

    const bool open =
        ImGui::TreeNodeEx(pass->name().c_str(), flags, "%s",
                          pass->name().c_str());

    if (!open) {
      continue;
    }

    const auto *pipeline = pass->editablePipeline();
    if (pipeline != nullptr) {
      ImGui::Text("Pipeline: %s", pipeline->desc().name.empty()
                                      ? "<unnamed>"
                                      : pipeline->desc().name.c_str());
    } else {
      ImGui::TextDisabled("Pipeline: none");
    }

    if (ImGui::TreeNodeEx("Reads", ImGuiTreeNodeFlags_SpanAvailWidth)) {
      if (pass->reads().empty()) {
        ImGui::TextDisabled("None");
      } else {
        for (const auto &resource : pass->reads()) {
          ImGui::BulletText("%s", resource.c_str());
        }
      }
      ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Writes", ImGuiTreeNodeFlags_SpanAvailWidth)) {
      if (pass->writes().empty()) {
        ImGui::TextDisabled("None");
      } else {
        for (const auto &resource : pass->writes()) {
          ImGui::BulletText("%s", resource.c_str());
        }
      }
      ImGui::TreePop();
    }

    ImGui::TreePop();
  }
}

} // namespace vkr::ui
