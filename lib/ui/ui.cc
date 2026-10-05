#include "vkr/ui/ui.hh"
#include "vkr/exec/capability.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>
#include <string_view>
#include <vector>

namespace vkr::ui {
namespace {

void checkVkResult(VkResult err) {
  if (err == VK_SUCCESS) {
    return;
  }

  std::fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if (err < 0) {
    std::abort();
  }
}

} // namespace

UI::UI(const core::Window &window, const core::Instance &instance,
       const core::Surface &surface, const core::Device &device,
       const core::CommandPool &commandPool, scene::Scene &scene,
       const util::AssetSystem &assetSystem, scene::CameraDesc &camera,
       exec::Pass &source,
       const pipeline::RenderPass &renderPass,
       const pipeline::DescriptorPool &descriptorPool, exec::Graph &graph,
       util::Timer &timer, UiDesc &desc,
       const core::CommandBuffers &commandBuffers)
    : window_(window), instance_(instance), surface_(surface), device_(device),
      command_pool_(commandPool), scene_(scene), asset_system_(assetSystem),
      camera_(camera), source_(source),
      render_pass_(renderPass), descriptor_pool_(descriptorPool),
      timer_(timer), command_buffers_(commandBuffers), desc_(desc) {
  if (command_buffers_.empty()) {
    VKR_UI_ERROR("UI requires initialized command buffers");
  }

  VKR_UI_INFO("Initializing ImGui UI...");
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  layout_mode_ = desc_.layoutMode;
  // Preserve a saved dockspace even when the first frame opens in fullscreen.
  dockspace_id_ = ImHashStr("DockSpace", 0, ImHashStr("DockSpace"));
  Theme::apply(desc_.theme);

  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  ImGui_ImplGlfw_InitForVulkan(window_.glfwWindow(), true);

  ImGui_ImplVulkan_PipelineInfo pipelineInfo{};
  pipelineInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  pipelineInfo.RenderPass = render_pass_.renderPass();

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Instance = instance_.instance();
  initInfo.PhysicalDevice = device_.physicalDevice();
  initInfo.Device = device_.device();
  initInfo.QueueFamily = device_.graphicsFamily();
  initInfo.Queue = device_.graphicsQueue();
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.DescriptorPool = descriptor_pool_.pool();
  initInfo.Allocator = nullptr;
  initInfo.MinImageCount = 2;
  initInfo.ImageCount = std::max(2U, command_buffers_.size());
  initInfo.CheckVkResultFn = checkVkResult;
  initInfo.PipelineInfoMain = pipelineInfo;

  ImGui_ImplVulkan_Init(&initInfo);

  std::vector<pipeline::DescriptorBinding> offscreenBindings = {
      {.name = "offscreen",
       .layout = {
           .binding = 0,
           .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
           .descriptorCount = 1,
           .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
       }}};

  offscreen_descriptor_layout_ =
      std::make_unique<pipeline::DescriptorSetLayout>(device_);
  offscreen_descriptor_layout_->update({.bindings = offscreenBindings});

  const auto target = source_.capability<exec::RenderTargetCapability>();
  if (!target) {
    VKR_UI_ERROR("UI source '{}' has no offscreen target", source_.name());
  }
  std::vector<pipeline::DescriptorSetWriteDesc> writes{};
  for (uint32_t frame = 0; frame < command_buffers_.size(); ++frame) {
    const auto &color = target->get().target(frame).color();
    if (!color.hasSampler()) {
      VKR_UI_ERROR("UI source '{}' color has no sampler", source_.name());
    }
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = color.sampler();
    imageInfo.imageView = color.imageView();
    imageInfo.imageLayout = color.desc().finalLayout;
    if (imageInfo.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    auto write = pipeline::DescriptorSetWriteDesc::forSet(frame);
    write.images.push_back(pipeline::DescriptorImageWriteDesc::one(
        0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo));
    writes.push_back(std::move(write));
  }

  offscreen_descriptor_sets_ =
      std::make_unique<pipeline::DescriptorSets>(device_);
  offscreen_descriptor_sets_->update(pipeline::DescriptorSetsDesc{
      .pool = descriptor_pool_.pool(),
      .layout = offscreen_descriptor_layout_->layout(),
      .setCount = command_buffers_.size(),
      .writes = std::move(writes),
  });

  viewport_panel_ = std::make_unique<ViewportPanel>(
      desc_.viewport, desc_.viewportFocused, desc_.viewportHovered);
  viewport_panel_->flipY(desc_.viewportFlipY);
  graph_panel_ = std::make_unique<ExecGraphPanel>(graph);
  assets_panel_ = std::make_unique<AssetsPanel>(asset_system_);
  camera_panel_ = std::make_unique<CameraPanel>(
      camera_, desc_.viewport, desc_.viewportFocused, desc_.viewportHovered);
  mesh_editor_panel_ = std::make_unique<MeshEditorPanel>(scene_);

  VKR_UI_INFO("Initializing FPS Panel...");
  fps_panel_ = std::make_unique<FPSPanel>(timer);
  fps_panel_->clear();
  VKR_UI_INFO("FPS Panel initialized successfully.");

  VKR_UI_INFO("Initializing Shader Editor...");
  shader_editor_ = std::make_unique<ShaderEditor>(graph);
  VKR_UI_INFO("Shader Editor initialized successfully.");

  VKR_UI_INFO("Initializing Logging Panel...");
  logging_panel_ = std::make_unique<LoggingPanel>();
  VKR_UI_INFO("Logging Panel initialized successfully.");

  VKR_UI_INFO("Initializing Resource Tree...");
  resource_tree_ = std::make_unique<ResourceTree>(scene_);
  VKR_UI_INFO("Resource Tree initialized successfully.");

  dock_components_ = {*viewport_panel_, *resource_tree_, *graph_panel_,
                      *assets_panel_,   *camera_panel_,  *mesh_editor_panel_,
                      *shader_editor_,  *fps_panel_,     *logging_panel_};

  VKR_UI_INFO("ImGui UI initialized successfully.");
}

UI::~UI() {
  shader_editor_.reset();
  logging_panel_.reset();
  fps_panel_.reset();
  mesh_editor_panel_.reset();
  camera_panel_.reset();
  assets_panel_.reset();
  graph_panel_.reset();
  resource_tree_.reset();
  viewport_panel_.reset();

  offscreen_descriptor_sets_.reset();
  offscreen_descriptor_layout_.reset();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void UI::render(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
  frame_index_ = frameIndex;
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  if (ImGui::IsKeyPressed(ImGuiKey_F11, false)) {
    switchLayoutMode();
  }

  renderMainMenu();

  switch (layout_mode_) {
  case LayoutMode::FullScreen:
    if (dockspace_id_ != 0) {
      ImGui::DockSpace(dockspace_id_, ImVec2(0.0f, 0.0f),
                       ImGuiDockNodeFlags_KeepAliveOnly);
    }
    renderFullScreen();
    break;
  case LayoutMode::Standard:
    renderStatusBar();
    renderDockspace();
    renderWorkspacePanels();
    break;
  }

  ImGui::Render();
  ImDrawData *drawData = ImGui::GetDrawData();
  ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
}

void UI::renderFullScreen() {
  const auto workRect =
      ImGui::GetCurrentContext()->Viewports[0]->GetBuildWorkRect();
  ImGui::SetNextWindowPos(workRect.Min);
  ImGui::SetNextWindowSize(workRect.GetSize());

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoScrollWithMouse;

  if (ImGui::Begin("Fullscreen Viewport", nullptr, flags)) {
    if (viewport_panel_) {
      VkDescriptorSet texture = VK_NULL_HANDLE;
      if (offscreen_descriptor_sets_ &&
          !offscreen_descriptor_sets_->sets().empty()) {
        texture = offscreen_descriptor_sets_->set(frame_index_);
      }

      viewport_panel_->renderFullscreen(texture);
    }
  }

  ImGui::End();
  ImGui::PopStyleVar(2);
}

void UI::renderDockspace() {
  const auto workRect =
      ImGui::GetCurrentContext()->Viewports[0]->GetBuildWorkRect();
  ImGui::SetNextWindowPos(workRect.Min);
  ImGui::SetNextWindowSize(workRect.GetSize());
  ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
  ImGui::Begin("DockSpace", nullptr, flags);
  ImGui::PopStyleVar(3);

  dockspace_id_ = ImGui::GetID("DockSpace");
  if (dock_layout_dirty_ || !ImGui::DockBuilderGetNode(dockspace_id_)) {
    setupDockingLayout();
    dock_layout_dirty_ = false;
  }
  ImGui::DockSpace(dockspace_id_, ImVec2(0.0f, 0.0f));

  ImGui::End();
}

void UI::setupDockingLayout() {
  const ImVec2 size = ImGui::GetContentRegionAvail();
  const float width = std::max(1.0f, size.x);
  const float height = std::max(1.0f, size.y);
  const float fontSize = ImGui::GetFontSize();
  const float leftWidth = std::min(width * 0.24f,
      std::clamp(width * 0.19f, fontSize * 15.0f, fontSize * 22.0f));
  const float rightWidth = std::min(width * 0.27f,
      std::clamp(width * 0.22f, fontSize * 18.0f, fontSize * 26.0f));
  const float bottomHeight = std::min(height * 0.30f,
      std::clamp(height * 0.20f, fontSize * 7.0f, fontSize * 13.0f));

  ImGui::DockBuilderRemoveNode(dockspace_id_);
  ImGui::DockBuilderAddNode(dockspace_id_, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodePos(dockspace_id_, ImGui::GetWindowPos());
  ImGui::DockBuilderSetNodeSize(dockspace_id_, size);

  ImGuiID center = dockspace_id_;
  ImGuiID bottom = 0;
  if (height >= fontSize * 24.0f) {
    bottom = ImGui::DockBuilderSplitNode(
        center, ImGuiDir_Down, bottomHeight / height, nullptr, &center);
  }
  ImGuiID left = center;
  ImGuiID right = center;
  if (width >= fontSize * 36.0f) {
    left = ImGui::DockBuilderSplitNode(
        center, ImGuiDir_Left, leftWidth / width, nullptr, &center);
    right = left;
    if (width >= fontSize * 64.0f) {
      right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right,
          rightWidth / (width - leftWidth), nullptr, &center);
    }
  }
  if (bottom == 0) {
    bottom = center;
  }

  ImGui::DockBuilderDockWindow(resource_tree_->name().c_str(), left);
  ImGui::DockBuilderDockWindow(assets_panel_->name().c_str(), left);
  ImGui::DockBuilderDockWindow(camera_panel_->name().c_str(), right);
  ImGui::DockBuilderDockWindow(mesh_editor_panel_->name().c_str(), right);
  ImGui::DockBuilderDockWindow(viewport_panel_->name().c_str(), center);
  ImGui::DockBuilderDockWindow(shader_editor_->name().c_str(), center);
  ImGui::DockBuilderDockWindow(graph_panel_->name().c_str(), center);
  ImGui::DockBuilderDockWindow(logging_panel_->name().c_str(), bottom);
  ImGui::DockBuilderDockWindow(fps_panel_->name().c_str(), bottom);

  ImGui::DockBuilderGetNode(center)->SelectedTabId =
      ImHashStr("#TAB", 0, ImHashStr(viewport_panel_->name().c_str()));
  if (bottom != center) {
    ImGui::DockBuilderGetNode(bottom)->SelectedTabId =
        ImHashStr("#TAB", 0, ImHashStr(logging_panel_->name().c_str()));
  }
  if (left != center) {
    ImGui::DockBuilderGetNode(left)->SelectedTabId =
        ImHashStr("#TAB", 0, ImHashStr(resource_tree_->name().c_str()));
  }
  if (right != center && right != left) {
    ImGui::DockBuilderGetNode(right)->SelectedTabId =
        ImHashStr("#TAB", 0, ImHashStr(camera_panel_->name().c_str()));
  }

  ImGui::DockBuilderFinish(dockspace_id_);
}

void UI::resetDockingLayout() noexcept {
  for (auto component : dock_components_) {
    component.get().resetOpen();
  }

  dock_layout_dirty_ = true;
}

void UI::renderMainMenu() {
  if (!ImGui::BeginMainMenuBar()) {
    return;
  }

  auto brandColor = Theme::accentColor(desc_.theme.accent);
  if (!desc_.theme.dark) {
    brandColor.x *= 0.62f;
    brandColor.y *= 0.62f;
    brandColor.z *= 0.62f;
  }
  ImGui::TextColored(brandColor, "vkr");
  ImGui::SameLine(0.0f, ImGui::GetFontSize());

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Exit")) {
      glfwSetWindowShouldClose(window_.glfwWindow(), GLFW_TRUE);
    }

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View")) {
    if (ImGui::MenuItem("Fullscreen Viewport", "F11",
                        layout_mode_ == LayoutMode::FullScreen)) {
      layoutMode(LayoutMode::FullScreen);
    }

    if (ImGui::MenuItem("Editor Workspace", "F11",
                        layout_mode_ == LayoutMode::Standard)) {
      layoutMode(LayoutMode::Standard);
    }

    if (ImGui::MenuItem("Reset Dock Layout")) {
      resetDockingLayout();
      layoutMode(LayoutMode::Standard);
    }

    ImGui::Separator();

    for (auto component : dock_components_) {
      auto &panel = component.get();

      if (ImGui::MenuItem(panel.name().c_str(), nullptr, panel.open())) {
        panel.openRef() = !panel.open();
        if (panel.open()) {
          layoutMode(LayoutMode::Standard);
        }
      }
    }

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Appearance")) {
    renderThemeControls();
    ImGui::EndMenu();
  }

  const std::string_view modeLabel = layout_mode_ == LayoutMode::Standard
                                        ? "Viewport  F11" : "Editor  F11";
  const float buttonWidth = ImGui::CalcTextSize(modeLabel.data()).x +
                            ImGui::GetStyle().FramePadding.x * 2.0f;
  const float right = ImGui::GetWindowWidth() - buttonWidth -
                      ImGui::GetStyle().WindowPadding.x;
  if (right > ImGui::GetCursorPosX() + ImGui::GetFontSize()) {
    ImGui::SetCursorPosX(right);
    if (ImGui::Button(modeLabel.data())) {
      switchLayoutMode();
    }
  }

  ImGui::EndMainMenuBar();
}

void UI::renderStatusBar() {
  const ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavFocus |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
  if (ImGui::BeginViewportSideBar("##EditorStatus", ImGui::GetMainViewport(),
                                 ImGuiDir_Down, ImGui::GetFrameHeight(), flags)) {
    if (ImGui::BeginMenuBar()) {
      std::array<char, 96> timing{};
      const float fps = timer_.fps();
      if (fps > 0.0f) {
        std::snprintf(timing.data(), timing.size(), "%.1f FPS   %.2f ms",
                      fps, 1000.0f / fps);
      } else {
        std::snprintf(timing.data(), timing.size(), "-- FPS   -- ms");
      }
      const float timingWidth = ImGui::CalcTextSize(timing.data()).x;
      const float right = ImGui::GetWindowWidth() - timingWidth -
                          ImGui::GetStyle().WindowPadding.x;
      if (right > ImGui::GetFontSize() * 24.0f) {
        const auto position = ImGui::GetCursorScreenPos();
        const auto windowPosition = ImGui::GetWindowPos();
        ImGui::PushClipRect(position,
            ImVec2(windowPosition.x + right - ImGui::GetFontSize(),
                   position.y + ImGui::GetFrameHeight()), true);
        ImGui::TextDisabled("Output: %s", source_.name().c_str());
        if (ImGui::GetCursorPosX() + ImGui::GetFontSize() * 12.0f < right) {
          ImGui::TextDisabled("%.0f x %.0f", desc_.viewport.width,
                              desc_.viewport.height);
        }
        ImGui::PopClipRect();
      }
      if (right > 0.0f) {
        ImGui::SetCursorPosX(right);
      }
      ImGui::TextDisabled("%s", timing.data());
      ImGui::EndMenuBar();
    }
  }
  ImGui::End();
}

void UI::renderWorkspacePanels() {
  desc_.viewport = {};
  desc_.viewportFocused = false;
  desc_.viewportHovered = false;
  VkDescriptorSet texture = VK_NULL_HANDLE;
  if (offscreen_descriptor_sets_ &&
      !offscreen_descriptor_sets_->sets().empty()) {
    texture = offscreen_descriptor_sets_->set(frame_index_);
  }

  if (viewport_panel_) {
    viewport_panel_->setTexture(texture);
  }

  for (auto component : dock_components_) {
    auto &panel = component.get();
    if (panel.open()) {
      panel.renderWindow();
    }
  }
}

void UI::renderThemeControls() {
  bool changed = false;

  ImGui::SeparatorText("Accent");

  auto accentItem = [&](const char *label, ThemeAccent accent) {
    if (ImGui::MenuItem(label, nullptr, desc_.theme.accent == accent)) {
      desc_.theme.accent = accent;
      changed = true;
    }
  };

  accentItem("Blue", ThemeAccent::Blue);
  accentItem("Red", ThemeAccent::Red);
  accentItem("Green", ThemeAccent::Green);
  accentItem("Purple", ThemeAccent::Purple);
  accentItem("Amber", ThemeAccent::Amber);

  ImGui::SeparatorText("Mode");

  if (ImGui::MenuItem("Dark", nullptr, desc_.theme.dark)) {
    desc_.theme.dark = true;
    changed = true;
  }

  if (ImGui::MenuItem("Light", nullptr, !desc_.theme.dark)) {
    desc_.theme.dark = false;
    changed = true;
  }

  ImGui::SeparatorText("Shape");
  ImGui::PushItemWidth(140.0f);

  changed |= ImGui::SliderFloat("Rounding", &desc_.theme.rounding, 0.0f, 10.0f,
                                "%.1f");
  changed |=
      ImGui::SliderFloat("Alpha", &desc_.theme.alpha, 0.35f, 1.0f, "%.2f");

  ImGui::PopItemWidth();

  if (changed) {
    Theme::apply(desc_.theme);
  }
}

} // namespace vkr::ui
