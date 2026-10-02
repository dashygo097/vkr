#pragma once

#include "vkr/exec/pass.hh"
#include "vkr/exec/render/executor.hh"

namespace vkr::exec {

class PresentPass final : public Pass, public PresentCapability {
public:
  explicit PresentPass(RenderExecutor &executor);

  void create() override {}
  void destroy() override {}
  void record() override;
  void present() override;

private:
  // dependencies
  RenderExecutor &executor_;
};

} // namespace vkr::exec
