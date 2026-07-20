#pragma once

#include "vkr/exec/render/pass.hh"
#include "vkr/exec/render/executor.hh"

namespace vkr::exec {

class PresentPass final : public Pass {
public:
  explicit PresentPass(Executor &executor);

  void create() override {}
  void destroy() override {}
  void record() override;
  void present() override;

  [[nodiscard]] auto presentsToSwapchain() const noexcept -> bool override {
    return true;
  }

private:
  Executor &executor_;
};

} // namespace vkr::exec
