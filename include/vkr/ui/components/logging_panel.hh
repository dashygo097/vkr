#pragma once

#include <imgui.h>

namespace vkr::ui {

class LoggingPanel {
public:
  LoggingPanel() = default;
  ~LoggingPanel() = default;

  void render();

private:
  bool auto_scroll_{true};
};

} // namespace vkr::ui
