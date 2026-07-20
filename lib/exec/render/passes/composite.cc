#include "vkr/exec/render/passes/composite.hh"

namespace vkr::exec {

CompositePass::CompositePass(Executor &executor, const core::Device &device,
                             const core::CommandPool &commandPool,
                             std::vector<FullscreenPassSource> sources)
    : FullscreenPass(executor, device, commandPool, std::move(sources)) {}

} // namespace vkr::exec
