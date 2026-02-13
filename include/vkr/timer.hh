#pragma once

#include <array>
#include <chrono>

namespace vkr {
static constexpr size_t FRAME_TIME_HISTORY_SIZE = 10;

class Timer {
public:
  Timer();

  void start();
  void beginFrame(); // Call at start of frame
  void update();     // Call after rendering
  void endFrame();   // Call at end of frame
  void reset();

  [[nodiscard]] float deltaTime() const noexcept { return delta_time_; }
  [[nodiscard]] float fps() const noexcept { return fps_; }
  [[nodiscard]] float elapsedTime() const noexcept { return elapsed_time_; }
  [[nodiscard]] uint64_t frameCount() const noexcept { return frame_count_; }
  void maxFPS(float maxFPS) noexcept { max_fps_ = maxFPS; }
  [[nodiscard]] float maxFPS() const noexcept { return max_fps_; }

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;
  std::chrono::time_point<std::chrono::high_resolution_clock> _lastTime;
  std::chrono::time_point<std::chrono::high_resolution_clock> _frameStartTime;

  float delta_time_{0.0f};
  float fps_{0.0f};
  float elapsed_time_{0.0f};
  uint64_t frame_count_{0};

  // Frame time smoothing
  std::array<float, FRAME_TIME_HISTORY_SIZE> _frameTimeHistory{};
  size_t _frameTimeIndex{0};
  float max_fps_{0.0f};
};

} // namespace vkr
