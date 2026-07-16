#include "vkr/ui/ui.hh"
#include "vkr/core/core_utils.hh"
#include "vkr/logger.hh"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>
#include <vector>

namespace vkr::ui {

UI::UI(const core::Window &window, const core::Instance &instance,
       const core::Surface &surface, const core::Device &device,
       const core::CommandPool &commandPool,
       resource::ResourceManager &resourceManager,
       const util::AssetSystem &assetSystem,
       scene::CameraDesc &camera,
       resource::OffscreenTarget &offscreenTarget,
       const pipeline::RenderPass &renderPass,
       const pipeline::DescriptorPool &descriptorPool,
       render::RenderGraph &renderGraph, util::Timer &timer, UiDesc &desc)
    : window_(window), instance_(instance), surface_(surface), device_(device),
      command_pool_(commandPool), resource_manager_(resourceManager),
      asset_system_(assetSystem), camera_(camera),
      offscreen_target_(offscreenTarget), render_pass_(renderPass),
      descriptor_pool_(descriptorPool), render_graph_(renderGraph),
      timer_(timer), desc_(desc) {
  VKR_UI_INFO("Initializing ImGui UI...");
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  layout_mode_ = desc_.layoutMode;
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
  initInfo.MinImageCount = core::MAX_FRAMES_IN_FLIGHT;
  initInfo.ImageCount = core::MAX_FRAMES_IN_FLIGHT;
  initInfo.CheckVkResultFn = core::check_vk_result;
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

  VkDescriptorImageInfo offscreenImageInfo{};
  offscreenImageInfo.sampler = offscreen_target_.color().sampler();
  offscreenImageInfo.imageView = offscreen_target_.color().imageView();
  offscreenImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  auto offscreenWrite = pipeline::DescriptorSetWriteDesc::forSet(0);
  offscreenWrite.images.push_back(pipeline::DescriptorImageWriteDesc::one(
      0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, offscreenImageInfo));

  offscreen_descriptor_sets_ =
      std::make_unique<pipeline::DescriptorSets>(device_);
  offscreen_descriptor_sets_->update(pipeline::DescriptorSetsDesc{
      .pool = descriptor_pool_.pool(),
      .layout = offscreen_descriptor_layout_->layout(),
      .setCount = 1,
      .writes = {offscreenWrite},
  });

  viewport_panel_ = std::make_unique<ViewportPanel>(
      desc_.viewport, desc_.viewportFocused, desc_.viewportHovered);
  render_graph_panel_ = std::make_unique<RenderGraphPanel>(render_graph_);
  assets_panel_ = std::make_unique<AssetsPanel>(asset_system_);
  camera_panel_ = std::make_unique<CameraPanel>(
      camera_, desc_.viewport, desc_.viewportFocused, desc_.viewportHovered);
  mesh_editor_panel_ = std::make_unique<MeshEditorPanel>(resource_manager_);

  VKR_UI_INFO("Initializing FPS Panel...");
  fps_panel_ = std::make_unique<FPSPanel>(timer);
  fps_panel_->clear();
  VKR_UI_INFO("FPS Panel initialized successfully.");

  VKR_UI_INFO("Initializing Shader Editor...");
  shader_editor_ = std::make_unique<ShaderEditor>(render_graph_);
  VKR_UI_INFO("Shader Editor initialized successfully.");

  VKR_UI_INFO("Initializing Logging Panel...");
  logging_panel_ = std::make_unique<LoggingPanel>();
  VKR_UI_INFO("Logging Panel initialized successfully.");

  VKR_UI_INFO("Initializing Resource Tree...");
  resource_tree_ = std::make_unique<ResourceTree>(resource_manager_);
  VKR_UI_INFO("Resource Tree initialized successfully.");

  dock_components_ = {*viewport_panel_,     *resource_tree_,
                      *render_graph_panel_, *assets_panel_,
                      *camera_panel_,       *mesh_editor_panel_,
                      *shader_editor_,      *fps_panel_,
                      *logging_panel_};

  VKR_UI_INFO("ImGui UI initialized successfully.");
}

UI::~UI() {
  shader_editor_.reset();
  logging_panel_.reset();
  fps_panel_.reset();
  mesh_editor_panel_.reset();
  camera_panel_.reset();
  assets_panel_.reset();
  render_graph_panel_.reset();
  resource_tree_.reset();
  viewport_panel_.reset();

  offscreen_descriptor_sets_.reset();
  offscreen_descriptor_layout_.reset();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void UI::render(VkCommandBuffer commandBuffer) {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  switch (layout_mode_) {
  case LayoutMode::FullScreen:
    renderMainMenu();
    renderFullScreen();
    break;
  case LayoutMode::Standard:
    renderDockspace();
    renderWorkspacePanels();
    break;
  }

  ImGui::Render();
  ImDrawData *drawData = ImGui::GetDrawData();
  ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
}

void UI::renderFullScreen() {
  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  if (ImGui::Begin("Fullscreen Viewport", nullptr, flags)) {
    if (viewport_panel_) {
      VkDescriptorSet texture = VK_NULL_HANDLE;
      if (offscreen_descriptor_sets_ &&
          !offscreen_descriptor_sets_->sets().empty()) {
        texture = offscreen_descriptor_sets_->sets()[0];
      }

      viewport_panel_->renderFullscreen(texture);
    }
  }

  ImGui::End();
  ImGui::PopStyleVar(2);
}

void UI::renderDockspace() {
  static bool dockspaceOpen = true;
  static ImGuiDockNodeFlags dockSpaceFlags = ImGuiDockNodeFlags_None;

  renderMainMenu();

  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

  windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
  windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  windowFlags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  if (dockSpaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) {
    windowFlags |= ImGuiWindowFlags_NoBackground;
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace", &dockspaceOpen, windowFlags);
  ImGui::PopStyleVar();
  ImGui::PopStyleVar(2);

  ImGuiIO &io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ImGuiID dockSpaceId = ImGui::GetID("DockSpace");
    ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), dockSpaceFlags);

    if (dock_layout_dirty_) {
      dock_layout_dirty_ = false;
      setupDockingLayout();
    }
  }

  ImGui::End();
}

void UI::setupDockingLayout() {
  ImGuiID dockSpaceId = ImGui::GetID("DockSpace");
  ImGui::DockBuilderRemoveNode(dockSpaceId);
  ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(dockSpaceId, ImGui::GetMainViewport()->Size);

  ImGuiID dockLeft;
  ImGuiID dockLeftBottom;
  ImGuiID dockRight;
  ImGuiID dockRightBottom;
  ImGuiID dockRightTop;
  ImGuiID dockBottom;

  dockLeft = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Left, 0.2f,
                                         nullptr, &dockSpaceId);

  dockRight = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Right, 0.2f,
                                          nullptr, &dockSpaceId);

  dockRightTop = ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Up, 0.7f,
                                             nullptr, &dockRightBottom);

  dockBottom = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Down, 0.22f,
                                           nullptr, &dockSpaceId);
  dockLeftBottom = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.48f,
                                               nullptr, &dockLeft);

  ImGui::DockBuilderDockWindow(resource_tree_->name().c_str(), dockLeft);
  ImGui::DockBuilderDockWindow(assets_panel_->name().c_str(), dockLeft);
  ImGui::DockBuilderDockWindow(render_graph_panel_->name().c_str(),
                               dockLeftBottom);
  ImGui::DockBuilderDockWindow(camera_panel_->name().c_str(), dockLeftBottom);
  ImGui::DockBuilderDockWindow(mesh_editor_panel_->name().c_str(),
                               dockLeftBottom);
  ImGui::DockBuilderDockWindow(shader_editor_->name().c_str(), dockRightTop);
  ImGui::DockBuilderDockWindow(fps_panel_->name().c_str(), dockRightBottom);
  ImGui::DockBuilderDockWindow(logging_panel_->name().c_str(), dockBottom);
  ImGui::DockBuilderDockWindow(viewport_panel_->name().c_str(), dockSpaceId);

  ImGui::DockBuilderFinish(dockSpaceId);
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

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Exit")) {
      should_close_ = true;
    }

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View")) {
    if (ImGui::MenuItem("Fullscreen Viewport", nullptr,
                        layout_mode_ == LayoutMode::FullScreen)) {
      layout_mode_ = LayoutMode::FullScreen;
    }

    if (ImGui::MenuItem("Workspace", nullptr,
                        layout_mode_ == LayoutMode::Standard)) {
      layout_mode_ = LayoutMode::Standard;
    }

    if (ImGui::MenuItem("Reset Dock Layout")) {
      resetDockingLayout();
      layout_mode_ = LayoutMode::Standard;
    }

    ImGui::SeparatorText("Panels");

    for (auto component : dock_components_) {
      auto &panel = component.get();

      if (ImGui::MenuItem(panel.name().c_str(), nullptr, panel.open())) {
        panel.openRef() = !panel.open();
      }
    }

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Theme")) {
    renderThemeControls();
    ImGui::EndMenu();
  }

  ImGui::Separator();
  ImGui::TextDisabled("%.1f FPS", timer_.fps());

  ImGui::EndMainMenuBar();
}

void UI::renderWorkspacePanels() {
  VkDescriptorSet texture = VK_NULL_HANDLE;
  if (offscreen_descriptor_sets_ &&
      !offscreen_descriptor_sets_->sets().empty()) {
    texture = offscreen_descriptor_sets_->sets()[0];
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

  changed |=
      ImGui::SliderFloat("Rounding", &desc_.theme.rounding, 0.0f, 10.0f,
                         "%.1f");
  changed |=
      ImGui::SliderFloat("Alpha", &desc_.theme.alpha, 0.35f, 1.0f, "%.2f");

  ImGui::PopItemWidth();

  if (changed) {
    Theme::apply(desc_.theme);
  }
}

} // namespace vkr::ui
