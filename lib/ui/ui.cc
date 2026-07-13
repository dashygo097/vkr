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
       resource::OffscreenTarget &offscreenTarget,
       const pipeline::RenderPass &renderPass,
       const pipeline::DescriptorPool &descriptorPool,
       render::RenderGraph &renderGraph, util::Timer &timer, ThemeDesc &desc)
    : window_(window), instance_(instance), surface_(surface), device_(device),
      command_pool_(commandPool), resource_manager_(resourceManager),
      offscreen_target_(offscreenTarget), render_pass_(renderPass),
      descriptor_pool_(descriptorPool), render_graph_(renderGraph),
      timer_(timer), desc_(desc) {
  VKR_UI_INFO("Initializing ImGui UI...");
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  Theme::apply(desc_);

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

  VKR_UI_INFO("ImGui UI initialized successfully.");
}

UI::~UI() {
  shader_editor_.reset();
  logging_panel_.reset();
  fps_panel_.reset();
  resource_tree_.reset();

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
    renderMainViewport();
    renderGraphPanel();
    renderResourcePanel();
    renderPerformancePanel();
    renderShaderEditor();
    renderLoggingPanel();
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
    if (offscreen_descriptor_sets_ &&
        !offscreen_descriptor_sets_->sets().empty()) {
      ImGui::Image(
          reinterpret_cast<ImTextureID>(offscreen_descriptor_sets_->sets()[0]),
          ImGui::GetContentRegionAvail());
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

    static bool firstTime = true;
    if (firstTime) {
      firstTime = false;
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

  ImGuiID dockLeft, dockRight, dockBottomRight, dockShader, dockLogs, dockGraph;

  dockLeft = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Left, 0.2f,
                                         nullptr, &dockSpaceId);

  dockRight = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Right, 0.2f,
                                          nullptr, &dockSpaceId);

  dockShader = ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Up, 0.8f,
                                           nullptr, &dockBottomRight);

  dockLogs = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Down, 0.2f,
                                         nullptr, &dockSpaceId);
  dockGraph = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.45f,
                                          nullptr, &dockLeft);

  ImGui::DockBuilderDockWindow("Resources", dockLeft);
  ImGui::DockBuilderDockWindow("Render Graph", dockGraph);
  ImGui::DockBuilderDockWindow("Shader Editor", dockShader);
  ImGui::DockBuilderDockWindow("Performance", dockBottomRight);
  ImGui::DockBuilderDockWindow("Logging", dockLogs);
  ImGui::DockBuilderDockWindow("Viewport", dockSpaceId);

  ImGui::DockBuilderFinish(dockSpaceId);
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

void UI::renderMainViewport() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Viewport", nullptr, windowFlags)) {
    ImVec2 panelSize = ImGui::GetContentRegionAvail();

    if (panelSize.x < 1.0f) {
      panelSize.x = 1.0f;
    }

    if (panelSize.y < 1.0f) {
      panelSize.y = 1.0f;
    }

    if (offscreen_descriptor_sets_ &&
        !offscreen_descriptor_sets_->sets().empty()) {
      ImGui::Image(
          reinterpret_cast<ImTextureID>(offscreen_descriptor_sets_->sets()[0]),
          panelSize);
    } else {
      ImGui::TextDisabled("(no offscreen target)");
    }

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

    viewport_info_.x = windowPos.x + contentMin.x;
    viewport_info_.y = windowPos.y + contentMin.y;
    viewport_info_.width = contentMax.x - contentMin.x;
    viewport_info_.height = contentMax.y - contentMin.y;
    viewport_info_.isFocused =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    viewport_info_.isHovered = ImGui::IsWindowHovered();
  }

  ImGui::End();
  ImGui::PopStyleVar();
}

void UI::renderGraphPanel() {
  ImGuiWindowFlags windowFlags =
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

  if (ImGui::Begin("Render Graph", nullptr, windowFlags)) {
    const auto passes = render_graph_.passes();

    ImGui::SeparatorText("Passes");
    ImGui::TextDisabled("%zu total", passes.size());
    ImGui::Spacing();

    for (const auto *pass : passes) {
      if (pass == nullptr) {
        continue;
      }

      ImGuiTreeNodeFlags flags =
          ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

      const bool open = ImGui::TreeNodeEx(pass->name().c_str(), flags, "%s",
                                          pass->name().c_str());

      if (open) {
        const auto *pipeline = pass->editablePipeline();
        if (pipeline != nullptr) {
          ImGui::Text("Pipeline: %s", pipeline->desc().name.empty()
                                          ? "<unnamed>"
                                          : pipeline->desc().name.c_str());
        } else {
          ImGui::TextDisabled("Pipeline: none");
        }

        if (ImGui::TreeNodeEx("Reads", ImGuiTreeNodeFlags_SpanAvailWidth)) {
          if (pass->reads().empty()) {
            ImGui::TextDisabled("None");
          } else {
            for (const auto &resource : pass->reads()) {
              ImGui::BulletText("%s", resource.c_str());
            }
          }
          ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Writes", ImGuiTreeNodeFlags_SpanAvailWidth)) {
          if (pass->writes().empty()) {
            ImGui::TextDisabled("None");
          } else {
            for (const auto &resource : pass->writes()) {
              ImGui::BulletText("%s", resource.c_str());
            }
          }
          ImGui::TreePop();
        }

        ImGui::TreePop();
      }
    }
  }

  ImGui::End();
}

void UI::renderResourcePanel() {
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Resources", nullptr, windowFlags)) {
    if (resource_tree_) {
      resource_tree_->render();
    }
  }

  ImGui::End();
}

void UI::renderPerformancePanel() {
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Performance", nullptr, windowFlags)) {
    if (fps_panel_) {
      fps_panel_->render();
    }
  }

  ImGui::End();
}

void UI::renderShaderEditor() {
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Shader Editor", nullptr, windowFlags)) {
    if (shader_editor_) {
      shader_editor_->render();
    }
  }

  ImGui::End();
}

void UI::renderThemeControls() {
  bool changed = false;

  ImGui::SeparatorText("Accent");

  auto accentItem = [&](const char *label, ThemeAccent accent) {
    if (ImGui::MenuItem(label, nullptr, desc_.accent == accent)) {
      desc_.accent = accent;
      changed = true;
    }
  };

  accentItem("Blue", ThemeAccent::Blue);
  accentItem("Red", ThemeAccent::Red);
  accentItem("Green", ThemeAccent::Green);
  accentItem("Purple", ThemeAccent::Purple);
  accentItem("Amber", ThemeAccent::Amber);

  ImGui::SeparatorText("Mode");

  if (ImGui::MenuItem("Dark", nullptr, desc_.dark)) {
    desc_.dark = true;
    changed = true;
  }

  if (ImGui::MenuItem("Light", nullptr, !desc_.dark)) {
    desc_.dark = false;
    changed = true;
  }

  ImGui::SeparatorText("Shape");
  ImGui::PushItemWidth(140.0f);

  changed |=
      ImGui::SliderFloat("Rounding", &desc_.rounding, 0.0f, 10.0f, "%.1f");
  changed |= ImGui::SliderFloat("Alpha", &desc_.alpha, 0.35f, 1.0f, "%.2f");

  ImGui::PopItemWidth();

  if (changed) {
    Theme::apply(desc_);
  }
}

void UI::renderLoggingPanel() {
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Logging", nullptr, windowFlags)) {
    if (logging_panel_) {
      logging_panel_->render();
    }
  }

  ImGui::End();
}

} // namespace vkr::ui
