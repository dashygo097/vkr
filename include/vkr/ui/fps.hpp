#pragma once

class FPSPanel {
public:
  FPSPanel() = default;
  ~FPSPanel() = default;
  FPSPanel(const FPSPanel &) = delete;
  FPSPanel &operator=(const FPSPanel &) = delete;

  void render(float fps);
};
