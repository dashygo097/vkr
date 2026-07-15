#include "vkr/ui/components/ui_component.hh"

namespace vkr::ui {

UiComponent::UiComponent(std::string name, bool defaultOpen)
    : name_(std::move(name)), open_(defaultOpen), default_open_(defaultOpen) {}

void UiComponent::renderWindow() {
  if (ImGui::Begin(name_.c_str(), &open_, windowFlags())) {
    render();
  }

  ImGui::End();
}

} // namespace vkr::ui
