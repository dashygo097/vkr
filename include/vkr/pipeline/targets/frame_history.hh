#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/targets/offscreen.hh"
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace vkr::pipeline {

struct FrameHistoryTargetDesc {
  OffscreenTargetDesc target{};
  uint32_t frameCount{0};

  [[nodiscard]] static auto history(OffscreenTargetDesc desc,
                                    uint32_t frameCount = 0)
      -> FrameHistoryTargetDesc {
    FrameHistoryTargetDesc result{};
    result.target = std::move(desc);
    result.frameCount = frameCount;
    return result;
  }
};

class FrameHistoryTarget {
public:
  explicit FrameHistoryTarget(const core::Device &device);
  ~FrameHistoryTarget();

  FrameHistoryTarget(const FrameHistoryTarget &) = delete;
  auto operator=(const FrameHistoryTarget &) -> FrameHistoryTarget & = delete;

  void create();
  void destroy();
  void update(const FrameHistoryTargetDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const FrameHistoryTargetDesc & {
    return desc_;
  }

  [[nodiscard]] auto read() -> OffscreenTarget &;
  [[nodiscard]] auto read() const -> const OffscreenTarget &;

  [[nodiscard]] auto write() -> OffscreenTarget &;
  [[nodiscard]] auto write() const -> const OffscreenTarget &;

  [[nodiscard]] auto readForFrame(uint32_t frameIndex) -> OffscreenTarget &;
  [[nodiscard]] auto readForFrame(uint32_t frameIndex) const
      -> const OffscreenTarget &;

  [[nodiscard]] auto writeForFrame(uint32_t frameIndex) -> OffscreenTarget &;
  [[nodiscard]] auto writeForFrame(uint32_t frameIndex) const
      -> const OffscreenTarget &;

  [[nodiscard]] auto target(uint32_t index) -> OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t index) const -> const OffscreenTarget &;

  [[nodiscard]] auto targetCount() const noexcept -> uint32_t {
    return static_cast<uint32_t>(targets_.size());
  }

  [[nodiscard]] auto targetCountForFrames() const noexcept -> uint32_t {
    return desc_.frameCount;
  }

  [[nodiscard]] auto readIndexForFrame(uint32_t frameIndex) const noexcept
      -> uint32_t {
    const uint32_t count = targetCountForFrames();
    return (writeIndexForFrame(frameIndex) + count - 1U) % count;
  }

  [[nodiscard]] auto writeIndexForFrame(uint32_t frameIndex) const noexcept
      -> uint32_t {
    return frameIndex % targetCountForFrames();
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  FrameHistoryTargetDesc desc_{};
  std::vector<std::unique_ptr<OffscreenTarget>> targets_{};

  // helpers
  void ensureTargets();
};

} // namespace vkr::pipeline
