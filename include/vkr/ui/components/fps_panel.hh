#pragma once

#include <array>
#include <cstdint>

namespace vkr::ui {
static constexpr uint32_t FPS_PANEL_HISTORY_SIZE = 256;

class FPSPanel {
public:
  FPSPanel() = default;
  ~FPSPanel() = default;

  FPSPanel(const FPSPanel &) = delete;
  FPSPanel &operator=(const FPSPanel &) = delete;

  void clear();
  void render(float fps);

private:
  std::array<float, FPS_PANEL_HISTORY_SIZE> fps_history_{};
  uint32_t fps_index_{0};
  float sum_fps_{0.0f};
  float avg_fps_{0.0f};
  bool is_filled_{false};
};
} // namespace vkr::ui
