#include "vkr/ui/components/shader_editor.hh"
#include <imgui.h>

namespace vkr::ui {

static void renderCodeBlock(char *buf, size_t bufSize, const char *id) {
  ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
  ImVec2 size = ImVec2(-1.0f, -1.0f);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.85f, 1.0f));
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.Size > 1
                      ? ImGui::GetIO().Fonts->Fonts[1]
                      : ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::InputTextMultiline(id, buf, bufSize, size, flags);
  ImGui::PopFont();
  ImGui::PopStyleColor(2);
}

void ShaderEditor::render() {
  if (ImGui::BeginTabBar("ShaderTabs")) {
    if (ImGui::BeginTabItem("Vertex")) {
      active_tab_ = 0;
      renderCodeBlock(state_.vertBuffer.data(), state_.vertBuffer.size(),
                      "##vert_src");
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Fragment")) {
      active_tab_ = 1;
      renderCodeBlock(state_.fragBuffer.data(), state_.fragBuffer.size(),
                      "##frag_src");
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (!state_.statusMessage.empty()) {
    ImVec4 col = state_.statusIsError ? ImVec4(1, 0.3f, 0.3f, 1)
                                      : ImVec4(0.3f, 1, 0.5f, 1);
    ImGui::TextColored(col, "%s", state_.statusMessage.c_str());
    ImGui::Spacing();
  }

  bool canCompile = on_compile_ != nullptr;
  if (!canCompile)
    ImGui::BeginDisabled();

  if (ImGui::Button("Compile & Apply", ImVec2(140, 0))) {
    state_.statusMessage = "Compiling...";
    state_.statusIsError = false;
    on_compile_(std::string(state_.vertBuffer.data()),
                std::string(state_.fragBuffer.data()));
  }

  if (!canCompile)
    ImGui::EndDisabled();

  ImGui::SameLine();
  if (ImGui::Button("Clear Status", ImVec2(100, 0)))
    state_.statusMessage.clear();
}

} // namespace vkr::ui
