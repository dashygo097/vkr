#pragma once

#include "vkr/exec/render/passes/fullscreen.hh"

namespace vkr::exec {

class CompositePass final : public FullscreenPass {
public:
  CompositePass(Executor &executor, const core::Device &device,
                const core::CommandPool &commandPool,
                std::vector<FullscreenPassSource> sources);
};

} // namespace vkr::exec
