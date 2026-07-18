#pragma once

#include "vkr/render/passes/fullscreen.hh"

namespace vkr::render {

class PostProcessPass final : public FullscreenPass {
public:
  PostProcessPass(Renderer &renderer, const core::Device &device,
                  const core::CommandPool &commandPool,
                  FullscreenPassSource source);
};

} // namespace vkr::render
