#pragma once

#include "vkr/render/pass.hh"
#include "vkr/render/renderer.hh"

namespace vkr::render {

class PresentPass final : public RenderGraphPass {
public:
  explicit PresentPass(Renderer &renderer);

  void create() override {}
  void destroy() override {}
  void record() override;
  void present() override;

  [[nodiscard]] auto presentsToSwapchain() const noexcept -> bool override {
    return true;
  }

private:
  Renderer &renderer_;
};

} // namespace vkr::render
