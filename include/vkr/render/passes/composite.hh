#pragma once

#include "vkr/render/passes/fullscreen_pass.hh"

namespace vkr::render {

class CompositePass final : public FullscreenPass {
public:
  CompositePass(Renderer &renderer, const core::Device &device,
                const core::CommandPool &commandPool,
                std::vector<FullscreenPassSource> sources);
};

} // namespace vkr::render
