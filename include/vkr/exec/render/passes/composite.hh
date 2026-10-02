#pragma once

#include "vkr/exec/render/passes/fullscreen.hh"

namespace vkr::exec {

class CompositePass final : public FullscreenPass {
public:
  CompositePass(RenderExecutor &executor, const core::Device &device,
                const core::CommandPool &commandPool,
                std::vector<RenderPassSource> sources);
};

} // namespace vkr::exec
