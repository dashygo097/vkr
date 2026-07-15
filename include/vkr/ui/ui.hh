#pragma once

#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/core/window.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/render/graph.hh"
#include "vkr/resource/manager.hh"
#include "vkr/resource/targets/offscreen.hh"
#include "vkr/scene/camera.hh"
#include "vkr/ui/components/assets_panel.hh"
#include "vkr/ui/components/camera_panel.hh"
#include "vkr/ui/components/fps_panel.hh"
#include "vkr/ui/components/logging_panel.hh"
#include "vkr/ui/components/mesh_editor_panel.hh"
#include "vkr/ui/components/render_graph_panel.hh"
#include "vkr/ui/components/resource_tree.hh"
#include "vkr/ui/components/shader_editor.hh"
#include "vkr/ui/components/ui_component.hh"
#include "vkr/ui/components/viewport_panel.hh"
#include "vkr/ui/theme.hh"
#include "vkr/util/asset.hh"
#include "vkr/util/timer.hh"
#include <memory>
#include <vector>

namespace vkr::ui {

enum LayoutMode {
  FullScreen,
  Standard,
};

class UI {
public:
  UI(const core::Window &window, const core::Instance &instance,
     const core::Surface &surface, const core::Device &device,
     const core::CommandPool &commandPool,
     resource::ResourceManager &resourceManager,
     const util::AssetSystem &assetSystem,
     scene::CameraDesc &camera,
     resource::OffscreenTarget &offscreenTarget,
     const pipeline::RenderPass &renderPass,
     const pipeline::DescriptorPool &descriptorPool,
     render::RenderGraph &renderGraph, util::Timer &timer, ThemeDesc &desc);
  ~UI();

  UI(const UI &) = delete;
  auto operator=(const UI &) -> UI & = delete;

  void render(VkCommandBuffer commandBuffer);

  [[nodiscard]] auto desc() const noexcept -> const ThemeDesc & {
    return desc_;
  }

  void layoutMode(LayoutMode mode) noexcept { layout_mode_ = mode; }

  void switchLayoutMode() noexcept {
    switch (layout_mode_) {
    case LayoutMode::FullScreen:
      layout_mode_ = LayoutMode::Standard;
      break;
    case LayoutMode::Standard:
      layout_mode_ = LayoutMode::FullScreen;
      break;
    }
  }

  [[nodiscard]] auto shouldClose() const noexcept -> bool {
    return should_close_;
  }

  void viewport(const VkViewport &viewport) noexcept {
    viewport_ = viewport;
  }

  [[nodiscard]] auto layoutMode() const noexcept -> LayoutMode {
    return layout_mode_;
  }

  [[nodiscard]] auto viewport() const noexcept -> const VkViewport & {
    return viewport_;
  }

  [[nodiscard]] auto viewportFocused() const noexcept -> bool {
    return viewport_focused_;
  }

  [[nodiscard]] auto viewportHovered() const noexcept -> bool {
    return viewport_hovered_;
  }

private:
  // dependencies
  const core::Window &window_;
  const core::Instance &instance_;
  const core::Surface &surface_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  resource::ResourceManager &resource_manager_;
  const util::AssetSystem &asset_system_;
  scene::CameraDesc &camera_;
  resource::OffscreenTarget &offscreen_target_;
  const pipeline::RenderPass &render_pass_;
  const pipeline::DescriptorPool &descriptor_pool_;
  render::RenderGraph &render_graph_;
  util::Timer &timer_;

  // components
  ThemeDesc &desc_;
  std::unique_ptr<ViewportPanel> viewport_panel_;
  std::unique_ptr<ResourceTree> resource_tree_;
  std::unique_ptr<RenderGraphPanel> render_graph_panel_;
  std::unique_ptr<AssetsPanel> assets_panel_;
  std::unique_ptr<CameraPanel> camera_panel_;
  std::unique_ptr<MeshEditorPanel> mesh_editor_panel_;
  std::unique_ptr<FPSPanel> fps_panel_;
  std::unique_ptr<ShaderEditor> shader_editor_;
  std::unique_ptr<LoggingPanel> logging_panel_;
  std::unique_ptr<pipeline::DescriptorSetLayout> offscreen_descriptor_layout_;
  std::unique_ptr<pipeline::DescriptorSets> offscreen_descriptor_sets_;

  // state
  LayoutMode layout_mode_{LayoutMode::FullScreen};
  VkViewport viewport_{};
  bool viewport_focused_{false};
  bool viewport_hovered_{false};
  bool should_close_{false};
  bool dock_layout_dirty_{true};
  std::vector<UiComponent *> dock_components_{};

  // helpers
  void renderFullScreen();
  void renderDockspace();
  void setupDockingLayout();
  void resetDockingLayout() noexcept;
  void renderMainMenu();
  void renderWorkspacePanels();
  void renderThemeControls();
};

} // namespace vkr::ui
