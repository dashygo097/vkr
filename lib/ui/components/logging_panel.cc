#include "vkr/ui/components/logging_panel.hh"
#include "vkr/logger.hh"

namespace vkr::ui {

void LoggingPanel::render() {
  ImGui::Checkbox("Auto-scroll", &auto_scroll_);
  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    vkr::Logger::getUiSink()->clear();
  }

  ImGui::Separator();

  ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false,
                    ImGuiWindowFlags_HorizontalScrollbar);

  auto messages = vkr::Logger::getUiSink()->getMessages();

  for (const auto &msg : messages) {
    ImVec4 color;
    bool has_color = true;

    switch (msg.level) {
    case spdlog::level::trace:
      color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
      break;
    case spdlog::level::debug:
      color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
      break;
    case spdlog::level::info:
      color = ImVec4(0.6f, 1.0f, 0.6f, 1.0f);
      break;
    case spdlog::level::warn:
      color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
      break;
    case spdlog::level::err:
    case spdlog::level::critical:
      color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
      break;
    default:
      has_color = false;
      break;
    }

    if (has_color)
      ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(msg.text.c_str());
    if (has_color)
      ImGui::PopStyleColor();
  }

  if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
    ImGui::SetScrollHereY(1.0f);
  }

  ImGui::EndChild();
}

} // namespace vkr::ui
