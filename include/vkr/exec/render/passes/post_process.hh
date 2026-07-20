#pragma once

#include "vkr/exec/render/passes/fullscreen.hh"

namespace vkr::exec {

class PostProcessPass final : public FullscreenPass {
public:
  PostProcessPass(Executor &executor, const core::Device &device,
                  const core::CommandPool &commandPool,
                  FullscreenPassSource source);
};

} // namespace vkr::exec
