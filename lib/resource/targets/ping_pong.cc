#include "vkr/resource/targets/ping_pong.hh"
#include "vkr/logger.hh"

namespace vkr::resource {

PingPongTarget::PingPongTarget(const core::Device &device,
                               const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool) {
  ensureTargets();
}

PingPongTarget::~PingPongTarget() { destroy(); }

void PingPongTarget::create() {
  ensureTargets();

  for (auto &target : targets_) {
    target->update(desc_.target);
  }
}

void PingPongTarget::destroy() {
  targets_[0].reset();
  targets_[1].reset();
  read_index_ = 0;
  write_index_ = 1;
}

void PingPongTarget::update(const PingPongTargetDesc &desc) {
  desc_ = desc;
  create();
}

void PingPongTarget::swap() noexcept {
  std::swap(read_index_, write_index_);
}

auto PingPongTarget::read() -> OffscreenTarget & {
  return target(read_index_);
}

auto PingPongTarget::read() const -> const OffscreenTarget & {
  return target(read_index_);
}

auto PingPongTarget::write() -> OffscreenTarget & {
  return target(write_index_);
}

auto PingPongTarget::write() const -> const OffscreenTarget & {
  return target(write_index_);
}

auto PingPongTarget::readForFrame(uint32_t frameIndex) -> OffscreenTarget & {
  return target(readIndexForFrame(frameIndex));
}

auto PingPongTarget::readForFrame(uint32_t frameIndex) const
    -> const OffscreenTarget & {
  return target(readIndexForFrame(frameIndex));
}

auto PingPongTarget::writeForFrame(uint32_t frameIndex) -> OffscreenTarget & {
  return target(writeIndexForFrame(frameIndex));
}

auto PingPongTarget::writeForFrame(uint32_t frameIndex) const
    -> const OffscreenTarget & {
  return target(writeIndexForFrame(frameIndex));
}

auto PingPongTarget::target(uint32_t index) -> OffscreenTarget & {
  if (index >= targets_.size() || !targets_[index]) {
    VKR_RES_ERROR("PingPongTarget index {} out of range or not created",
                  index);
  }

  return *targets_[index];
}

auto PingPongTarget::target(uint32_t index) const -> const OffscreenTarget & {
  if (index >= targets_.size() || !targets_[index]) {
    VKR_RES_ERROR("PingPongTarget index {} out of range or not created",
                  index);
  }

  return *targets_[index];
}

void PingPongTarget::ensureTargets() {
  for (auto &target : targets_) {
    if (!target) {
      target = std::make_unique<OffscreenTarget>(device_, command_pool_);
    }
  }
}

} // namespace vkr::resource
