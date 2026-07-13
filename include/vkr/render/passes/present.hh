#pragma once

#include "vkr/render/pass.hh"

namespace vkr::render {

class PresentPass final : public RenderGraphPass {
public:
  PresentPass() = default;

  void create() override {}
  void destroy() override {}
  void update(const RenderGraphPassDesc &desc) override { setDesc(desc); }
  void record() override;

  [[nodiscard]] auto desc() const noexcept
      -> const RenderGraphPassDesc & override {
    return desc_;
  }

private:
  RenderGraphPassDesc desc_{};

  [[nodiscard]] auto mutableDesc() noexcept -> RenderGraphPassDesc & override {
    return desc_;
  }
};

} // namespace vkr::render
