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
  if (ImGui::Button("Clear")) {
    vkr::Logger::getUiSink()->clear();
  }
  ImGui::SameLine();
  ImGui::Checkbox("Auto-scroll", &auto_scroll_);
  const float filterWidth = ImGui::GetContentRegionAvail().x;
  if (filterWidth > ImGui::GetFontSize() * 24.0f) {
    ImGui::SameLine();
  }
  ImGui::SetNextItemWidth(-1.0f);
  if (ImGui::InputTextWithHint("##log_filter", "Filter messages...",
                              filter_.InputBuf, sizeof(filter_.InputBuf))) {
    filter_.Build();
  }

  ImGui::Separator();

  if (!ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), ImGuiChildFlags_None,
                         ImGuiWindowFlags_HorizontalScrollbar)) {
    ImGui::EndChild();
    return;
  }
  const bool wasAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;

  auto messages = vkr::Logger::getUiSink()->getMessages();

  const ImGuiStyle &style = ImGui::GetStyle();
  const ImVec4 textColor = style.Colors[ImGuiCol_Text];
  const ImVec4 disabledColor = style.Colors[ImGuiCol_TextDisabled];
  const ImVec4 accentColor = style.Colors[ImGuiCol_CheckMark];
  const bool lightBackground = style.Colors[ImGuiCol_WindowBg].x > 0.5f;

  for (const auto &msg : messages) {
    if (!filter_.PassFilter(msg.text.c_str())) {
      continue;
    }
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
      color = textColor;
      break;
    case spdlog::level::warn:
      color = lightBackground ? ImVec4(0.68f, 0.38f, 0.07f, 1.0f)
                              : ImVec4(1.0f, 0.78f, 0.35f, 1.0f);
      break;
    case spdlog::level::err:
    case spdlog::level::critical:
      color = lightBackground ? ImVec4(0.72f, 0.16f, 0.18f, 1.0f)
                              : ImVec4(1.0f, 0.48f, 0.48f, 1.0f);
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

  if (auto_scroll_ && wasAtBottom) {
    ImGui::SetScrollHereY(1.0f);
  }

  ImGui::EndChild();
}

} // namespace vkr::ui
