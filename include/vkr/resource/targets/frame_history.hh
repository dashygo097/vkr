#pragma once

#include "vkr/core/command/buffers.hh"
#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/targets/offscreen.hh"
#include <memory>
#include <vector>

namespace vkr::resource {

struct FrameHistoryTargetDesc {
  OffscreenTargetDesc target{};
};

class FrameHistoryTarget {
public:
  FrameHistoryTarget(const core::Device &device,
                     const core::CommandPool &commandPool);
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

  [[nodiscard]] static constexpr auto targetCountForFrames() noexcept
      -> uint32_t {
    return static_cast<uint32_t>(core::MAX_FRAMES_IN_FLIGHT);
  }

  [[nodiscard]] static constexpr auto
  readIndexForFrame(uint32_t frameIndex) noexcept -> uint32_t {
    const uint32_t count = targetCountForFrames();
    return (writeIndexForFrame(frameIndex) + count - 1U) % count;
  }

  [[nodiscard]] static constexpr auto
  writeIndexForFrame(uint32_t frameIndex) noexcept -> uint32_t {
    return frameIndex % targetCountForFrames();
  }

private:
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  FrameHistoryTargetDesc desc_{};
  std::vector<std::unique_ptr<OffscreenTarget>> targets_{};

  void ensureTargets();
};

} // namespace vkr::resource
