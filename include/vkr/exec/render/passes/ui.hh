#pragma once

#include "vkr/core/command/buffers.hh"
#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/core/surface.hh"
#include "vkr/core/swapchain.hh"
#include "vkr/core/window.hh"
#include "vkr/exec/graph.hh"
#include "vkr/exec/pass.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/exec/render/frame_buffer_set.hh"
#include "vkr/exec/render/targets/swapchain.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/scene/camera.hh"
#include "vkr/scene/scene.hh"
#include "vkr/ui/ui.hh"
#include "vkr/util/asset.hh"
#include "vkr/util/timer.hh"
#include <memory>

namespace vkr::exec {

class UiPass final : public Pass {
public:
  UiPass(RenderExecutor &executor, const core::Window &window,
         const core::Instance &instance, const core::Surface &surface,
         const core::Device &device, const core::CommandPool &commandPool,
         const core::CommandBuffers &commandBuffers,
         const core::Swapchain &swapchain, scene::Scene &scene,
         const util::AssetSystem &assetSystem, scene::CameraDesc &camera,
         Pass &source, Graph &graph, util::Timer &timer,
         ui::UiDesc &uiDesc);
  ~UiPass() override;

  UiPass(const UiPass &) = delete;
  auto operator=(const UiPass &) -> UiPass & = delete;

  void create() override;
  void destroy() noexcept override;
  void record() override;

private:
  // dependencies
  RenderExecutor &executor_;
  const core::Window &window_;
  const core::Instance &instance_;
  const core::Surface &surface_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  const core::CommandBuffers &command_buffers_;
  const core::Swapchain &swapchain_;
  scene::Scene &scene_;
  const util::AssetSystem &asset_system_;
  scene::CameraDesc &camera_;
  Pass &source_;
  Graph &graph_;
  util::Timer &timer_;
  ui::UiDesc &ui_desc_;

  // components
  std::unique_ptr<SwapchainTarget> target_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::unique_ptr<FramebufferSet> framebuffers_{};
  std::unique_ptr<pipeline::DescriptorPool> descriptor_pool_{};
  std::unique_ptr<ui::UI> ui_{};
};

} // namespace vkr::exec
