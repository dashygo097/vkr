#include "vkr/resource/targets/frame_history.hh"
#include "vkr/logger.hh"

namespace vkr::resource {

FrameHistoryTarget::FrameHistoryTarget(const core::Device &device,
                                       const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool) {
  ensureTargets();
}

FrameHistoryTarget::~FrameHistoryTarget() { destroy(); }

void FrameHistoryTarget::create() {
  ensureTargets();

  for (auto &target : targets_) {
    target->update(desc_.target);
  }
}

void FrameHistoryTarget::destroy() { targets_.clear(); }

void FrameHistoryTarget::update(const FrameHistoryTargetDesc &desc) {
  desc_ = desc;
  create();
}

auto FrameHistoryTarget::read() -> OffscreenTarget & {
  return readForFrame(0);
}

auto FrameHistoryTarget::read() const -> const OffscreenTarget & {
  return readForFrame(0);
}

auto FrameHistoryTarget::write() -> OffscreenTarget & {
  return writeForFrame(0);
}

auto FrameHistoryTarget::write() const -> const OffscreenTarget & {
  return writeForFrame(0);
}

auto FrameHistoryTarget::readForFrame(uint32_t frameIndex)
    -> OffscreenTarget & {
  return target(readIndexForFrame(frameIndex));
}

auto FrameHistoryTarget::readForFrame(uint32_t frameIndex) const
    -> const OffscreenTarget & {
  return target(readIndexForFrame(frameIndex));
}

auto FrameHistoryTarget::writeForFrame(uint32_t frameIndex)
    -> OffscreenTarget & {
  return target(writeIndexForFrame(frameIndex));
}

auto FrameHistoryTarget::writeForFrame(uint32_t frameIndex) const
    -> const OffscreenTarget & {
  return target(writeIndexForFrame(frameIndex));
}

auto FrameHistoryTarget::target(uint32_t index) -> OffscreenTarget & {
  if (index >= targets_.size() || !targets_[index]) {
    VKR_RES_ERROR("FrameHistoryTarget index {} out of range or not created",
                  index);
  }

  return *targets_[index];
}

auto FrameHistoryTarget::target(uint32_t index) const
    -> const OffscreenTarget & {
  if (index >= targets_.size() || !targets_[index]) {
    VKR_RES_ERROR("FrameHistoryTarget index {} out of range or not created",
                  index);
  }

  return *targets_[index];
}

void FrameHistoryTarget::ensureTargets() {
  targets_.resize(targetCountForFrames());

  for (auto &target : targets_) {
    if (!target) {
      target = std::make_unique<OffscreenTarget>(device_, command_pool_);
    }
  }
}

} // namespace vkr::resource
