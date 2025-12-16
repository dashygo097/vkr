#pragma once

#include <chrono>

namespace vkr {

class Timer {
public:
  Timer();
  ~Timer() = default;

  void start();
  void update();
  void reset();

  [[nodiscard]] float deltaTime() const noexcept { return _deltaTime; }
  [[nodiscard]] float fps() const noexcept { return _fps; }
  [[nodiscard]] float elapsedTime() const noexcept { return _elapsedTime; }
  [[nodiscard]] uint64_t frameCount() const noexcept { return _frameCount; }

private:
  // components
  std::chrono::high_resolution_clock::time_point _startTime;
  std::chrono::high_resolution_clock::time_point _lastTime;
  float _deltaTime{0.0f};
  float _fps{0.0f};
  float _elapsedTime{0.0f};
  uint64_t _frameCount{0};
};

} // namespace vkr
