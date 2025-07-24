#pragma once

namespace vkr {

class FPSCounter {
public:
  FPSCounter();
  ~FPSCounter();

  void start();
  void update();
  void reset();

  [[nodiscard]] float fps() const noexcept { return _fps; }

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
  std::chrono::time_point<std::chrono::steady_clock> last_time;
  int frame_count;
  float _fps;
};
} // namespace vkr
