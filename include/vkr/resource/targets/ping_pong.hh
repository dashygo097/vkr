#pragma once

#include "vkr/core/command/command_buffer.hh"
#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/targets/offscreen.hh"
#include <array>
#include <memory>

namespace vkr::resource {

static_assert(core::MAX_FRAMES_IN_FLIGHT == 2,
              "PingPongTarget frame-indexed descriptors assume two frames in "
              "flight");

struct PingPongTargetDesc {
  OffscreenTargetDesc target{};
};

class PingPongTarget {
public:
  PingPongTarget(const core::Device &device,
                 const core::CommandPool &commandPool);
  ~PingPongTarget();

  PingPongTarget(const PingPongTarget &) = delete;
  auto operator=(const PingPongTarget &) -> PingPongTarget & = delete;

  void create();
  void destroy();
  void update(const PingPongTargetDesc &desc);
  void swap() noexcept;

  [[nodiscard]] auto desc() const noexcept -> const PingPongTargetDesc & {
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

  [[nodiscard]] auto readIndex() const noexcept -> uint32_t {
    return read_index_;
  }

  [[nodiscard]] auto writeIndex() const noexcept -> uint32_t {
    return write_index_;
  }

  [[nodiscard]] static auto readIndexForFrame(uint32_t frameIndex) noexcept
      -> uint32_t {
    return frameIndex % 2U;
  }

  [[nodiscard]] static auto writeIndexForFrame(uint32_t frameIndex) noexcept
      -> uint32_t {
    return 1U - readIndexForFrame(frameIndex);
  }

private:
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  PingPongTargetDesc desc_{};
  std::array<std::unique_ptr<OffscreenTarget>, 2> targets_{};
  uint32_t read_index_{0};
  uint32_t write_index_{1};

  void ensureTargets();
};

} // namespace vkr::resource
