#include "vkr/render/passes/present.hh"

namespace vkr::render {

PresentPass::PresentPass(Renderer &renderer) : renderer_(renderer) {
  read("swapchain");
}

void PresentPass::record() {
  // Queue presentation happens after the command buffer has been submitted.
  // This pass is still part of the graph so dependency ordering remains
  // explicit, but it records no commands.
}

void PresentPass::present() {
  renderer_.presentFrame();
}

} // namespace vkr::render
