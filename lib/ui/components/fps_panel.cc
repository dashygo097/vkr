#include "vkr/ui/components/fps_panel.hh"
#include <algorithm>
#include <imgui.h>

namespace vkr::ui {

static auto withAlpha(ImVec4 color, float alpha) -> ImVec4 {
  color.w = alpha;
  return color;
}

static auto mixColor(const ImVec4 &a, const ImVec4 &b, float t) -> ImVec4 {
  return ImVec4{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

FPSPanel::FPSPanel(util::Timer &timer) : timer_(timer) {
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

  const ImGuiStyle &style = ImGui::GetStyle();
  const ImVec4 textColor = style.Colors[ImGuiCol_Text];
  const ImVec4 accentColor = style.Colors[ImGuiCol_CheckMark];

  const ImVec4 goodColor =
      withAlpha(mixColor(textColor, accentColor, 0.65f), 0.90f);
  const ImVec4 warnColor = ImVec4(1.00f, 0.82f, 0.28f, 0.90f);
  const ImVec4 badColor = ImVec4(1.00f, 0.35f, 0.35f, 0.90f);

  ImVec4 fpsColor = fps > 59.0f   ? goodColor
                    : fps > 29.0f ? warnColor
                                  : badColor;

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

  ImGui::PushStyleColor(ImGuiCol_FrameBg,
                        withAlpha(style.Colors[ImGuiCol_FrameBg], 0.55f));
  ImGui::PushStyleColor(ImGuiCol_PlotLines, withAlpha(accentColor, 0.80f));

  ImGui::PlotLines("##FPSGraph", fps_history_.data(), FPS_PANEL_HISTORY_SIZE,
                   fps_index_, nullptr, 0.0f, 200.0f, ImVec2(0, 45));

  ImGui::PopStyleColor(2);
}

} // namespace vkr::ui
