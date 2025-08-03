#include "vkr/components/fps_counter.hpp"

namespace vkr {

FPSCounter::FPSCounter() {}

FPSCounter::~FPSCounter() {}

void FPSCounter::start() {
  start_time = std::chrono::high_resolution_clock::now();
  frame_count = 0;
}

void FPSCounter::update() {
  auto now = std::chrono::steady_clock::now();
  frame_count++;
  if (now - last_time >= std::chrono::seconds(1)) {
    auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(now - start_time)
            .count();
    _fps = static_cast<float>(frame_count) / elapsed;
    last_time = now;
    frame_count = 0;
  }
}

void FPSCounter::reset() {
  start_time = std::chrono::high_resolution_clock::now();
  last_time = std::chrono::steady_clock::now();
  frame_count = 0;
  _fps = 0.0f;
}
} // namespace vkr
