#include "vkr/exec/render/passes/source.hh"
#include "vkr/exec/render/passes/feedback_fullscreen.hh"
#include "vkr/exec/render/passes/fullscreen.hh"
#include "vkr/exec/render/passes/raster.hh"

namespace vkr::exec {

RenderPassSource::RenderPassSource(RasterPass &source)
    : source_(std::ref(source)) {}

RenderPassSource::RenderPassSource(FullscreenPass &source)
    : source_(std::ref(source)) {}

RenderPassSource::RenderPassSource(FeedbackFullscreenPass &source)
    : source_(std::ref(source)) {}

auto RenderPassSource::target() -> OffscreenTarget & { return target(0); }

auto RenderPassSource::target() const -> const OffscreenTarget & {
  return target(0);
}

auto RenderPassSource::target(uint32_t frameIndex) -> OffscreenTarget & {
  return std::visit(
      [frameIndex](auto source) -> OffscreenTarget & {
        return source.get().target(frameIndex);
      },
      source_);
}

auto RenderPassSource::target(uint32_t frameIndex) const
    -> const OffscreenTarget & {
  return std::visit(
      [frameIndex](auto source) -> const OffscreenTarget & {
        return source.get().target(frameIndex);
      },
      source_);
}

} // namespace vkr::exec
