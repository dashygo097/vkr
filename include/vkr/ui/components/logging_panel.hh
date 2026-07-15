#pragma once

#include "vkr/ui/components/ui_component.hh"
#include <imgui.h>

namespace vkr::ui {

class LoggingPanel final : public UiComponent {
public:
  LoggingPanel();
  ~LoggingPanel() = default;

private:
  void render();

  bool auto_scroll_{true};
};

} // namespace vkr::ui
