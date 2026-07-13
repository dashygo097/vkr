#include "vkr/render/passes/composite.hh"

namespace vkr::render {

CompositePass::CompositePass(Renderer &renderer, const core::Device &device,
                             const core::CommandPool &commandPool,
                             std::vector<FullscreenPassSource> sources)
    : FullscreenPass(renderer, device, commandPool, std::move(sources)) {}

} // namespace vkr::render
