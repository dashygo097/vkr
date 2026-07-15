#include "vkr/ui/components/logging_panel.hh"
#include "vkr/logger.hh"

namespace vkr::ui {

static auto withAlpha(ImVec4 color, float alpha) -> ImVec4 {
  color.w = alpha;
  return color;
}

static auto mixColor(const ImVec4 &a, const ImVec4 &b, float t) -> ImVec4 {
  return ImVec4{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

LoggingPanel::LoggingPanel() : UiComponent("Logging") {}

void LoggingPanel::render() {
  ImGui::Checkbox("Auto-scroll", &auto_scroll_);
  ImGui::SameLine();

  if (ImGui::Button("Clear")) {
    vkr::Logger::getUiSink()->clear();
  }

  ImGui::Separator();

  ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), true,
                    ImGuiWindowFlags_HorizontalScrollbar);

  auto messages = vkr::Logger::getUiSink()->getMessages();

  const ImGuiStyle &style = ImGui::GetStyle();
  const ImVec4 textColor = style.Colors[ImGuiCol_Text];
  const ImVec4 disabledColor = style.Colors[ImGuiCol_TextDisabled];
  const ImVec4 accentColor = style.Colors[ImGuiCol_CheckMark];

  for (const auto &msg : messages) {
    ImVec4 color;
    bool has_color = true;

    switch (msg.level) {
    case spdlog::level::trace:
      color = disabledColor;
      break;
    case spdlog::level::debug:
      color = withAlpha(mixColor(textColor, accentColor, 0.65f), 1.0f);
      break;
    case spdlog::level::info:
      color = withAlpha(
          mixColor(textColor, ImVec4(0.45f, 1.00f, 0.55f, 1.0f), 0.55f), 1.0f);
      break;
    case spdlog::level::warn:
      color = ImVec4(1.0f, 0.78f, 0.25f, 1.0f);
      break;
    case spdlog::level::err:
    case spdlog::level::critical:
      color = ImVec4(1.0f, 0.32f, 0.32f, 1.0f);
      break;
    default:
      has_color = false;
      break;
    }

    if (has_color) {
      ImGui::PushStyleColor(ImGuiCol_Text, color);
    }

    ImGui::TextUnformatted(msg.text.c_str());

    if (has_color) {
      ImGui::PopStyleColor();
    }
  }

  if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
    ImGui::SetScrollHereY(1.0f);
  }

  ImGui::EndChild();
}

} // namespace vkr::ui
