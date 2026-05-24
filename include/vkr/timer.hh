#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace vkr {

static constexpr size_t FRAME_TIME_HISTORY_SIZE = 60;

class Timer {
public:
  Timer();

  void start();
  void beginFrame();
  void update();
  void endFrame();
  void reset();

  void maxFPS(float maxFPS) noexcept { max_fps_ = maxFPS; }

  [[nodiscard]] auto deltaTime() const noexcept -> float { return delta_time_; }

  [[nodiscard]] auto rawDeltaTime() const noexcept -> float {
    return raw_delta_time_;
  }

  [[nodiscard]] auto fps() const noexcept -> float { return fps_; }

  [[nodiscard]] auto rawFPS() const noexcept -> float { return raw_fps_; }

  [[nodiscard]] auto elapsedTime() const noexcept -> float {
    return elapsed_time_;
  }

  [[nodiscard]] auto frameCount() const noexcept -> uint64_t {
    return frame_count_;
  }

  [[nodiscard]] auto maxFPS() const noexcept -> float { return max_fps_; }

private:
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;

  TimePoint start_time_{};
  TimePoint frame_start_time_{};

  float raw_delta_time_{0.0f};
  float delta_time_{0.0f};

  float raw_fps_{0.0f};
  float fps_{0.0f};

  float elapsed_time_{0.0f};
  float max_fps_{0.0f};

  uint64_t frame_count_{0};

  std::array<float, FRAME_TIME_HISTORY_SIZE> frame_time_history_{};
  size_t frame_time_index_{0};
  size_t frame_time_count_{0};

  void pushFrameTime(float frameTime);
};

} // namespace vkr
