#include "vkr/util/timer.hh"
#include <algorithm>
#include <thread>

namespace vkr::util {

Timer::Timer() { start(); }

void Timer::start() {
  auto now = Clock::now();

  start_time_ = now;
  frame_start_time_ = now;

  raw_delta_time_ = 0.0f;
  delta_time_ = 0.0f;

  raw_fps_ = 0.0f;
  fps_ = 0.0f;

  elapsed_time_ = 0.0f;
  frame_count_ = 0;

  frame_time_history_.fill(0.0f);
  frame_time_index_ = 0;
  frame_time_count_ = 0;
}

void Timer::beginFrame() { frame_start_time_ = Clock::now(); }

void Timer::update() {
  auto now = Clock::now();
  std::chrono::duration<float> elapsed = now - start_time_;
  elapsed_time_ = elapsed.count();
}

void Timer::endFrame() {
  if (max_fps_ > 0.0f) {
    const double targetFrameSeconds = 1.0 / static_cast<double>(max_fps_);
    const auto targetFrameDuration =
        std::chrono::duration<double>(targetFrameSeconds);

    const auto targetEndTime =
        frame_start_time_ +
        std::chrono::duration_cast<Clock::duration>(targetFrameDuration);

    while (true) {
      auto now = Clock::now();

      if (now >= targetEndTime) {
        break;
      }

      auto remaining = targetEndTime - now;

      if (remaining > std::chrono::milliseconds(2)) {
        std::this_thread::sleep_for(remaining - std::chrono::milliseconds(1));
      } else {
        std::this_thread::yield();
      }
    }
  }

  auto frame_end_time = Clock::now();

  std::chrono::duration<float> frameDuration =
      frame_end_time - frame_start_time_;

  raw_delta_time_ = frameDuration.count();

  constexpr float MAX_DELTA_TIME = 0.1f;
  raw_delta_time_ = std::min(raw_delta_time_, MAX_DELTA_TIME);

  if (raw_delta_time_ > 0.0f) {
    pushFrameTime(raw_delta_time_);
  }

  raw_fps_ = raw_delta_time_ > 0.0f ? 1.0f / raw_delta_time_ : 0.0f;
  fps_ = delta_time_ > 0.0f ? 1.0f / delta_time_ : 0.0f;

  std::chrono::duration<float> elapsed = frame_end_time - start_time_;
  elapsed_time_ = elapsed.count();

  frame_count_++;
}

void Timer::reset() { start(); }

void Timer::pushFrameTime(float frameTime) {
  frame_time_history_[frame_time_index_] = frameTime;
  frame_time_index_ = (frame_time_index_ + 1) % FRAME_TIME_HISTORY_SIZE;

  if (frame_time_count_ < FRAME_TIME_HISTORY_SIZE) {
    frame_time_count_++;
  }

  float sum = 0.0f;
  for (size_t i = 0; i < frame_time_count_; i++) {
    sum += frame_time_history_[i];
  }

  delta_time_ = sum / static_cast<float>(frame_time_count_);
}

} // namespace vkr::util
