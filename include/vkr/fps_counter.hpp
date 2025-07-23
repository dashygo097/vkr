#pragma once

namespace vkr {

class FPSCounter {
public:
  FPSCounter();
  ~FPSCounter();

  void start();
  void update();
  void reset();

  float getFPS() const { return fps; }

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
  std::chrono::time_point<std::chrono::steady_clock> last_time;
  int frame_count;
  float fps;
};
} // namespace vkr
