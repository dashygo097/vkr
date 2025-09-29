#pragma once

namespace vkr {
class FPSPanel {
public:
  FPSPanel() = default;
  ~FPSPanel() = default;
  FPSPanel(const FPSPanel &) = delete;
  FPSPanel &operator=(const FPSPanel &) = delete;

  void render();
};
} // namespace vkr
