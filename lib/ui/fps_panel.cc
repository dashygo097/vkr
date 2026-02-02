#include "vkr/ui/fps_panel.hh"
#include <imgui.h>

namespace vkr::ui {

void FPSPanel::render(float fps) {
  static float fps_history[256] = {};
  static int fps_index = 0;

  fps_history[fps_index] = fps;
  fps_index = (fps_index + 1) % 256;

  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.05f, 0.05f, 0.05f, 0.30f));
  ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.70f, 1.00f, 0.85f, 0.70f));

  ImGui::PlotLines("##FPSGraph", fps_history, 256, fps_index, nullptr, 0.0f,
                   120.0f, ImVec2(0, 45));

  ImGui::PopStyleColor(2);

  float avg_fps = 0.0f;
  for (int i = 0; i < 256; i++) {
    avg_fps += fps_history[i];
  }
  avg_fps /= 256.0f;

  ImGui::Text("Average: %.1f FPS", avg_fps);
}
} // namespace vkr::ui
