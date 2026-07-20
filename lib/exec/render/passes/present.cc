#include "vkr/exec/render/passes/present.hh"

namespace vkr::exec {

PresentPass::PresentPass(Executor &executor) : executor_(executor) {
  read("swapchain");
}

void PresentPass::record() {
  // Queue presentation happens after the command buffer has been submitted.
  // This pass is still part of the graph so dependency ordering remains
  // explicit, but it records no commands.
}

void PresentPass::present() {
  executor_.presentFrame();
}

} // namespace vkr::exec
