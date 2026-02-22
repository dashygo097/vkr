#include "vkr/ui/components/shader_editor.hh"
#include <imgui.h>

namespace vkr::ui {

static void renderCodeBlock(char *buf, size_t bufSize, const char *id) {
  ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.85f, 1.0f));
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.Size > 1
                      ? ImGui::GetIO().Fonts->Fonts[1]
                      : ImGui::GetIO().Fonts->Fonts[0]);
  ImGui::InputTextMultiline(id, buf, bufSize, ImVec2(-1.0f, -1.0f), flags);
  ImGui::PopFont();
  ImGui::PopStyleColor(2);
}

void ShaderEditor::render() {
  const float itemSpacing = ImGui::GetStyle().ItemSpacing.y;
  const float framePadding = ImGui::GetStyle().FramePadding.y;
  const float separatorH = 1.0f;

  const float statusH = state_.statusMessage.empty()
                            ? 0.0f
                            : ImGui::GetTextLineHeight() + itemSpacing;
  const float bottomBarH = itemSpacing + separatorH + itemSpacing + statusH +
                           ImGui::GetFrameHeight() + itemSpacing;

  if (ImGui::BeginTabBar("ShaderTabs")) {
    if (ImGui::BeginTabItem("Vertex")) {
      active_tab_ = 0;
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Fragment")) {
      active_tab_ = 1;
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  const float codeH = ImGui::GetContentRegionAvail().y - bottomBarH;

  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
  if (ImGui::BeginChild("##code_area", ImVec2(-1.0f, codeH), false,
                        ImGuiWindowFlags_NoScrollbar)) {
    if (active_tab_ == 0)
      renderCodeBlock(state_.vertBuffer.data(), state_.vertBuffer.size(),
                      "##vert_src");
    else
      renderCodeBlock(state_.fragBuffer.data(), state_.fragBuffer.size(),
                      "##frag_src");
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (!state_.statusMessage.empty()) {
    ImVec4 col = state_.statusIsError ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f)
                                      : ImVec4(0.3f, 1.0f, 0.5f, 1.0f);
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
