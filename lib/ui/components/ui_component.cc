#include "vkr/ui/components/ui_component.hh"

namespace vkr::ui {

UiComponent::UiComponent(std::string name, bool defaultOpen)
    : name_(std::move(name)), open_(defaultOpen), default_open_(defaultOpen) {}

void UiComponent::renderWindow() {
  ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 32.0f,
                                 ImGui::GetFontSize() * 24.0f),
                           ImGuiCond_FirstUseEver);
  if (ImGui::Begin(name_.c_str(), &open_,
                    windowFlags() | ImGuiWindowFlags_NoCollapse)) {
    render();
  }

  ImGui::End();
}

} // namespace vkr::ui
