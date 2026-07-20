#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/core/window.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/exec/render/graph.hh"
#include "vkr/scene/scene.hh"
#include "vkr/exec/render/targets/offscreen.hh"
#include "vkr/scene/camera.hh"
#include "vkr/ui/components/assets_panel.hh"
#include "vkr/ui/components/camera_panel.hh"
#include "vkr/ui/components/fps_panel.hh"
#include "vkr/ui/components/logging_panel.hh"
#include "vkr/ui/components/mesh_editor_panel.hh"
#include "vkr/ui/components/exec_graph_panel.hh"
#include "vkr/ui/components/resource_tree.hh"
#include "vkr/ui/components/shader_editor.hh"
#include "vkr/ui/components/ui_component.hh"
#include "vkr/ui/components/viewport_panel.hh"
#include "vkr/ui/theme.hh"
#include "vkr/util/asset.hh"
#include "vkr/util/timer.hh"
#include <functional>
#include <memory>
#include <vector>

namespace vkr::ui {

enum LayoutMode {
  FullScreen,
  Standard,
};

struct UiDesc {
  LayoutMode layoutMode{LayoutMode::FullScreen};
  ThemeDesc theme{};
  VkViewport viewport{};
  bool viewportFocused{false};
  bool viewportHovered{false};
  bool viewportFlipY{false};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    switch (layoutMode) {
    case LayoutMode::FullScreen:
    case LayoutMode::Standard:
      return theme.isValid();
    }

    return false;
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("layoutMode", layoutMode);
    ar("theme", theme);
  }
};

class UI {
public:
  UI(const core::Window &window, const core::Instance &instance,
     const core::Surface &surface, const core::Device &device,
     const core::CommandPool &commandPool,
     scene::Scene &scene,
     const util::AssetSystem &assetSystem, scene::CameraDesc &camera,
     exec::OffscreenTarget &offscreenTarget,
     const pipeline::RenderPass &renderPass,
     const pipeline::DescriptorPool &descriptorPool,
     exec::RenderGraph &graph, util::Timer &timer, UiDesc &desc);
  ~UI();

  UI(const UI &) = delete;
  auto operator=(const UI &) -> UI & = delete;

  void render(VkCommandBuffer commandBuffer);

  [[nodiscard]] auto desc() const noexcept -> const UiDesc & { return desc_; }

  void layoutMode(LayoutMode mode) noexcept {
    layout_mode_ = mode;
    desc_.layoutMode = mode;
  }

  void switchLayoutMode() noexcept {
    switch (layout_mode_) {
    case LayoutMode::FullScreen:
      layout_mode_ = LayoutMode::Standard;
      break;
    case LayoutMode::Standard:
      layout_mode_ = LayoutMode::FullScreen;
      break;
    }

    desc_.layoutMode = layout_mode_;
  }

  [[nodiscard]] auto shouldClose() const noexcept -> bool {
    return should_close_;
  }

  void viewport(const VkViewport &viewport) noexcept {
    desc_.viewport = viewport;
  }

  [[nodiscard]] auto layoutMode() const noexcept -> LayoutMode {
    return layout_mode_;
  }

  [[nodiscard]] auto viewport() const noexcept -> const VkViewport & {
    return desc_.viewport;
  }

  [[nodiscard]] auto viewportFocused() const noexcept -> bool {
    return desc_.viewportFocused;
  }

  [[nodiscard]] auto viewportHovered() const noexcept -> bool {
    return desc_.viewportHovered;
  }

private:
  // dependencies
  const core::Window &window_;
  const core::Instance &instance_;
  const core::Surface &surface_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  scene::Scene &scene_;
  const util::AssetSystem &asset_system_;
  scene::CameraDesc &camera_;
  exec::OffscreenTarget &offscreen_target_;
  const pipeline::RenderPass &render_pass_;
  const pipeline::DescriptorPool &descriptor_pool_;
  exec::RenderGraph &graph_;
  util::Timer &timer_;

  // components
  UiDesc &desc_;
  std::unique_ptr<ViewportPanel> viewport_panel_;
  std::unique_ptr<ResourceTree> resource_tree_;
  std::unique_ptr<ExecGraphPanel> graph_panel_;
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
  bool should_close_{false};
  bool dock_layout_dirty_{true};
  std::vector<std::reference_wrapper<UiComponent>> dock_components_{};

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
