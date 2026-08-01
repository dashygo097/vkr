#pragma once

#include "vkr/exec/render/targets/offscreen.hh"
#include <functional>
#include <variant>

namespace vkr::exec {

class RasterPass;
class FullscreenPass;
class FeedbackFullscreenPass;

struct RenderPassSource {
  using Source = std::variant<std::reference_wrapper<RasterPass>,
                              std::reference_wrapper<FullscreenPass>,
                              std::reference_wrapper<FeedbackFullscreenPass>>;

  explicit RenderPassSource(RasterPass &source);
  explicit RenderPassSource(FullscreenPass &source);
  explicit RenderPassSource(FeedbackFullscreenPass &source);

  [[nodiscard]] auto target() -> OffscreenTarget &;
  [[nodiscard]] auto target() const -> const OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t frameIndex) -> OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t frameIndex) const
      -> const OffscreenTarget &;

private:
  Source source_;
};

} // namespace vkr::exec
