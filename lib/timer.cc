#include "vkr/timer.hh"

namespace vkr {

Timer::Timer() {
  _startTime = std::chrono::high_resolution_clock::now();
  _lastTime = _startTime;
}

void Timer::start() {
  _startTime = std::chrono::high_resolution_clock::now();
  _lastTime = _startTime;
  _deltaTime = 0.0f;
  _fps = 0.0f;
  _elapsedTime = 0.0f;
  _frameCount = 0;
}

void Timer::update() {
  auto now = std::chrono::high_resolution_clock::now();

  std::chrono::duration<float> delta = now - _lastTime;
  _deltaTime = delta.count();

  std::chrono::duration<float> elapsed = now - _startTime;
  _elapsedTime = elapsed.count();

  _fps = (_deltaTime > 0.0f) ? (1.0f / _deltaTime) : 0.0f;

  _lastTime = now;
  _frameCount++;
}

void Timer::reset() { start(); }

} // namespace vkr
