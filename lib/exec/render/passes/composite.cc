#include "vkr/exec/render/passes/composite.hh"

namespace vkr::exec {

CompositePass::CompositePass(RenderExecutor &executor,
                             const core::Device &device,
                             const core::CommandPool &commandPool,
                             std::vector<RenderPassSource> sources)
    : FullscreenPass(executor, device, commandPool, std::move(sources)) {}

} // namespace vkr::exec
