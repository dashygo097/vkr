#pragma once

#include "vkr/render/pass.hh"

namespace vkr::render {

class PresentPass final : public RenderGraphPass {
public:
  PresentPass() = default;

  void create() override {}
  void destroy() override {}
  void record() override;
};

} // namespace vkr::render
