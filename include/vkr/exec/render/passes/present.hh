#pragma once

#include "vkr/exec/render/executor.hh"
#include "vkr/exec/render/pass.hh"

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
  // dependencies
  Executor &executor_;
};

} // namespace vkr::exec
