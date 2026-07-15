#pragma once

#include "vkr/ui/components/ui_component.hh"
#include "vkr/util/timer.hh"
#include <array>
#include <cstdint>

namespace vkr::ui {
static constexpr uint32_t FPS_PANEL_HISTORY_SIZE = 256;

class FPSPanel final : public UiComponent {
public:
  explicit FPSPanel(util::Timer &timer);
  ~FPSPanel() = default;

  FPSPanel(const FPSPanel &) = delete;
  auto operator=(const FPSPanel &) -> FPSPanel & = delete;

  void clear();

private:
  void render();

  // dependencies
  util::Timer &timer_;

  // components
  std::array<float, FPS_PANEL_HISTORY_SIZE> fps_history_{};
  uint32_t fps_index_{0};
  float sum_fps_{0.0f};
  float avg_fps_{0.0f};
  bool is_filled_{false};
  float max_fps_edit_{60.0f};
};
} // namespace vkr::ui
