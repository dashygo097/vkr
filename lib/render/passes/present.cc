#include "vkr/render/passes/present.hh"

namespace vkr::render {

void PresentPass::record() {
  // Presentation is submitted by Renderer::endFrame. This graph node keeps the
  // final dependency edge explicit for passes that write the swapchain.
}

} // namespace vkr::render
