#include "vkr/ui/components/fps_panel.hh"
#include <algorithm>
#include <imgui.h>

namespace vkr::ui {

FPSPanel::FPSPanel(Timer &timer) : timer_(timer) {
  clear();

  if (timer_.maxFPS() > 0.0f) {
    max_fps_edit_ = timer_.maxFPS();
  }
}

void FPSPanel::clear() {
  fps_history_.fill(0.0f);
  fps_index_ = 0;
  sum_fps_ = 0.0f;
  avg_fps_ = 0.0f;
  is_filled_ = false;
}

void FPSPanel::render() {
  float fps = timer_.fps();
  float oldFPS = fps_history_[fps_index_];
  bool unlimited = timer_.maxFPS() <= 0.0f;

  sum_fps_ = sum_fps_ - oldFPS + fps;

  fps_history_[fps_index_] = fps;
  fps_index_ = (fps_index_ + 1) % FPS_PANEL_HISTORY_SIZE;

  if (fps_index_ == 0) {
    is_filled_ = true;
  }

  uint32_t count = is_filled_ ? FPS_PANEL_HISTORY_SIZE : fps_index_;
  avg_fps_ = (count > 0) ? (sum_fps_ / count) : 0.0f;

  ImVec4 fpsColor = fps > 59   ? ImVec4(0.60f, 1.00f, 0.80f, 0.85f)
                    : fps > 29 ? ImVec4(1.00f, 0.95f, 0.55f, 0.85f)
                               : ImVec4(1.00f, 0.60f, 0.60f, 0.85f);

  ImGui::TextColored(fpsColor, "FPS: %.1f", fps);

  if (fps > 0.0f) {
    ImGui::Text("Frame Time: %.2f ms", 1000.0f / fps);
  } else {
    ImGui::Text("Frame Time: -- ms");
  }

  ImGui::Text("Average: %.1f FPS / Maximum %.1f FPS", avg_fps_,
              timer_.maxFPS());

  ImGui::Separator();

  if (ImGui::Checkbox("Unlimited FPS", &unlimited)) {
    if (unlimited) {
      timer_.maxFPS(0.0f);
    } else {
      if (max_fps_edit_ <= 0.0f) {
        max_fps_edit_ = 60.0f;
      }
      timer_.maxFPS(max_fps_edit_);
    }
  }

  if (!unlimited) {
    if (ImGui::SliderFloat("Maximum FPS", &max_fps_edit_, 1.0f, 500.0f,
                           "%.0f")) {
      max_fps_edit_ = std::clamp(max_fps_edit_, 1.0f, 500.0f);
      timer_.maxFPS(max_fps_edit_);
    }
  }

  ImGui::Separator();

  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.05f, 0.05f, 0.05f, 0.30f));
  ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.70f, 1.00f, 0.85f, 0.70f));

  ImGui::PlotLines("##FPSGraph", fps_history_.data(), FPS_PANEL_HISTORY_SIZE,
                   fps_index_, nullptr, 0.0f, 200.0f, ImVec2(0, 45));

  ImGui::PopStyleColor(2);
}

} // namespace vkr::ui
