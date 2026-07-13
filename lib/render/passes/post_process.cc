#include "vkr/render/passes/post_process.hh"

namespace vkr::render {

PostProcessPass::PostProcessPass(Renderer &renderer,
                                 const core::Device &device,
                                 const core::CommandPool &commandPool,
                                 FullscreenPassSource source)
    : FullscreenPass(renderer, device, commandPool, {source}) {}

} // namespace vkr::render
