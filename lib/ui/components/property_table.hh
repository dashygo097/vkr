#pragma once

#include <algorithm>
#include <imgui.h>
#include <string>
#include <string_view>

namespace vkr::ui {

// Shared panel layout, kept private to the UI implementation.
inline auto beginPropertyTable(const std::string &id) -> bool {
  const float width = ImGui::GetContentRegionAvail().x;
  const bool stacked = width < ImGui::GetFontSize() * 16.0f;
  if (!ImGui::BeginTable(id.c_str(), stacked ? 1 : 2,
                        ImGuiTableFlags_SizingStretchProp |
                            ImGuiTableFlags_NoSavedSettings |
                            ImGuiTableFlags_NoPadOuterX)) {
    return false;
  }
  if (!stacked) {
    const float labelWidth = std::min(ImGui::GetFontSize() * 9.0f, width * 0.4f);
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed,
                           labelWidth);
  }
  ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
  return true;
}

inline void propertyRow(std::string_view label) {
  const bool stacked = ImGui::TableGetColumnCount() == 1;
  ImGui::TableNextRow(0, stacked ? 0.0f : ImGui::GetFrameHeight());
  ImGui::TableSetColumnIndex(0);
  if (!stacked) {
    ImGui::AlignTextToFramePadding();
  }
  ImGui::PushTextWrapPos();
  ImGui::TextDisabled("%.*s", static_cast<int>(label.size()), label.data());
  ImGui::PopTextWrapPos();
  if (stacked) {
    ImGui::TableNextRow(0, ImGui::GetFrameHeight());
  }
  ImGui::TableSetColumnIndex(stacked ? 0 : 1);
  ImGui::SetNextItemWidth(-1.0f);
}

inline void propertyRow(std::string_view label, std::string_view value) {
  propertyRow(label);
  ImGui::PushID(label.data(), label.data() + label.size());
  ImGui::AlignTextToFramePadding();
  ImGui::TextWrapped("%.*s", static_cast<int>(value.size()), value.data());
  if (ImGui::BeginPopupContextItem("##property_actions")) {
    if (ImGui::MenuItem("Copy value")) {
      ImGui::SetClipboardText(std::string{value}.c_str());
    }
    ImGui::EndPopup();
  }
  ImGui::PopID();
}

} // namespace vkr::ui
