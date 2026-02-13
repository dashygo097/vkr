#include "vkr/timer.hh"
#include <algorithm>
#include <thread>

namespace vkr {

Timer::Timer() {
  _startTime = std::chrono::high_resolution_clock::now();
  _lastTime = _startTime;
  _frameStartTime = _startTime;
  _frameTimeHistory.fill(0.0f);
}

void Timer::start() {
  _startTime = std::chrono::high_resolution_clock::now();
  _lastTime = _startTime;
  _frameStartTime = _startTime;
  delta_time_ = 0.0f;
  fps_ = 0.0f;
  elapsed_time_ = 0.0f;
  frame_count_ = 0;
  _frameTimeHistory.fill(0.0f);
  _frameTimeIndex = 0;
}

void Timer::beginFrame() {
  _frameStartTime = std::chrono::high_resolution_clock::now();
}

void Timer::update() {
  auto now = std::chrono::high_resolution_clock::now();

  std::chrono::duration<float> delta = now - _lastTime;
  float rawDeltaTime = delta.count();

  constexpr float MAX_DELTA_TIME = 0.1f;
  rawDeltaTime = std::min(rawDeltaTime, MAX_DELTA_TIME);

  _frameTimeHistory[_frameTimeIndex] = rawDeltaTime;
  _frameTimeIndex = (_frameTimeIndex + 1) % FRAME_TIME_HISTORY_SIZE;

  float smoothedDeltaTime = 0.0f;
  for (float frameTime : _frameTimeHistory) {
    smoothedDeltaTime += frameTime;
  }
  smoothedDeltaTime /= FRAME_TIME_HISTORY_SIZE;

  delta_time_ = smoothedDeltaTime;

  std::chrono::duration<float> elapsed = now - _startTime;
  elapsed_time_ = elapsed.count();

  fps_ = (rawDeltaTime > 0.0f) ? (1.0f / rawDeltaTime) : 0.0f;

  _lastTime = now;
  frame_count_++;
}

void Timer::endFrame() {
  if (max_fps_ <= 0.0f) {
    return;
  }

  float targetFrameTime = 1.0f / max_fps_;

  while (true) {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = now - _frameStartTime;
    float remainingTime = targetFrameTime - elapsed.count();

    if (remainingTime <= 0.0f) {
      break;
    }

    if (remainingTime > 0.001f) {
      auto sleepTime = std::chrono::duration<float>(remainingTime - 0.001f);
      std::this_thread::sleep_for(sleepTime);
    } else {
      std::this_thread::yield();
    }
  }
}

void Timer::reset() { start(); }

} // namespace vkr
