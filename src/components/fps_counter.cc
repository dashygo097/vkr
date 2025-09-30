#include "vkr/components/fps_counter.hh"

namespace vkr {
FPSCounter::FPSCounter() {}
FPSCounter::~FPSCounter() {}

void FPSCounter::start() {
  start_time = std::chrono::high_resolution_clock::now();
}

void FPSCounter::update() {
  auto now = std::chrono::steady_clock::now();
  std::chrono::duration<float> delta = now - last_time;
  last_time = now;
  _fps = 1.0f / delta.count();
}

void FPSCounter::reset() {
  start_time = std::chrono::high_resolution_clock::now();
  last_time = std::chrono::steady_clock::now();
  _fps = 0.0f;
}
} // namespace vkr
