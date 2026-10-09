#pragma once

#include "vkr/core/command/buffers.hh"
#include "vkr/core/device.hh"
#include "vkr/core/instance.hh"
#include "vkr/core/window.hh"
#include "vkr/exec/graph.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/pipeline/targets/offscreen.hh"
#include "vkr/scene/camera.hh"
#include "vkr/scene/scene.hh"
#include "vkr/ui/components/assets_panel.hh"
#include "vkr/ui/components/camera_panel.hh"
#include "vkr/ui/components/exec_graph_panel.hh"
#include "vkr/ui/components/fps_panel.hh"
#include "vkr/ui/components/inspector_panel.hh"
#include "vkr/ui/components/logging_panel.hh"
#include "vkr/ui/components/mesh_editor_panel.hh"
#include "vkr/ui/components/resource_tree.hh"
#include "vkr/ui/components/shader_editor.hh"
#include "vkr/ui/components/ui_component.hh"
#include "vkr/ui/components/viewport_panel.hh"
#include "vkr/ui/selection.hh"
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
  LayoutMode layoutMode{LayoutMode::Standard};
  ThemeDesc theme{};
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
  explicit UI(const core::Window &window, const core::Instance &instance,
              const core::Device &device, scene::Scene &scene,
              const util::AssetSystem &assetSystem, scene::Camera &camera,
              exec::Pass &source, const pipeline::RenderPass &renderPass,
              const pipeline::DescriptorPool &descriptorPool,
              exec::Graph &graph, util::Timer &timer,
              const core::CommandBuffers &commandBuffers);
  ~UI();

  UI(const UI &) = delete;
  auto operator=(const UI &) -> UI & = delete;

  void create();
  void destroy() noexcept;
  void update(const UiDesc &desc);
  void prepare(uint32_t frameIndex);
  void render(VkCommandBuffer commandBuffer, uint32_t frameIndex);

  [[nodiscard]] auto valid() const noexcept -> bool { return created_; }

  [[nodiscard]] auto desc() const noexcept -> const UiDesc & { return desc_; }

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

  void viewport(const VkViewport &viewport) noexcept { viewport_ = viewport; }

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

  [[nodiscard]] auto theme() const noexcept -> const ThemeDesc & {
    return theme_;
  }

private:
  // dependencies
  const core::Window &window_;
  const core::Instance &instance_;
  const core::Device &device_;
  scene::Scene &scene_;
  const util::AssetSystem &asset_system_;
  scene::Camera &camera_;
  exec::Pass &source_;
  const pipeline::RenderPass &render_pass_;
  const pipeline::DescriptorPool &descriptor_pool_;
  exec::Graph &graph_;
  util::Timer &timer_;
  const core::CommandBuffers &command_buffers_;

  // components
  UiDesc desc_{};
  std::unique_ptr<ImGuiContext, decltype(&ImGui::DestroyContext)> context_{
      nullptr, &ImGui::DestroyContext};
  std::unique_ptr<ViewportPanel> viewport_panel_;
  std::unique_ptr<ResourceTree> resource_tree_;
  std::unique_ptr<ExecGraphPanel> graph_panel_;
  std::unique_ptr<AssetsPanel> assets_panel_;
  std::unique_ptr<CameraPanel> camera_panel_;
  std::unique_ptr<MeshEditorPanel> mesh_editor_panel_;
  std::unique_ptr<InspectorPanel> inspector_panel_;
  std::unique_ptr<FPSPanel> fps_panel_;
  std::unique_ptr<ShaderEditor> shader_editor_;
  std::unique_ptr<LoggingPanel> logging_panel_;
  std::unique_ptr<pipeline::DescriptorSetLayout> offscreen_descriptor_layout_;
  std::unique_ptr<pipeline::DescriptorSets> offscreen_descriptor_sets_;

  // state
  Selection selection_{};
  ThemeDesc theme_{};
  VkViewport viewport_{};
  bool viewport_focused_{false};
  bool viewport_hovered_{false};
  bool glfw_initialized_{false};
  bool vulkan_initialized_{false};
  bool created_{false};
  LayoutMode layout_mode_{LayoutMode::Standard};
  uint32_t frame_index_{0};
  ImGuiID dockspace_id_{0};
  bool dock_layout_dirty_{false};
  float dpi_scale_{1.0f};
  bool theme_dirty_{true};
  std::vector<std::reference_wrapper<UiComponent>> dock_components_{};

  // helpers
  void updateTheme();
  void select(Selection selection);
  void renderFullScreen();
  void renderDockspace();
  void setupDockingLayout();
  void resetDockingLayout() noexcept;
  void renderMainMenu();
  void renderStatusBar();
  void renderWorkspacePanels();
  void renderThemeControls();
};

} // namespace vkr::ui
