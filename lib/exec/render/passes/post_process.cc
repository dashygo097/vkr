#include "vkr/exec/render/passes/post_process.hh"

namespace vkr::exec {

PostProcessPass::PostProcessPass(Executor &executor,
                                 const core::Device &device,
                                 const core::CommandPool &commandPool,
                                 FullscreenPassSource source)
    : FullscreenPass(executor, device, commandPool, {source}) {}

} // namespace vkr::exec
