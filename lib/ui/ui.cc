#include "vkr/ui/ui.hh"
#include "vkr/core/command_buffer.hh"
#include "vkr/core/core_utils.hh"
#include "vkr/core/queue_families.hh"
#include "vkr/logger.hh"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

namespace vkr::ui {

UI::UI(const core::Window &window, const core::Instance &instance,
       const core::Surface &surface, const core::Device &device,
       const core::CommandPool &commandPool,
       const pipeline::RenderPass &renderPass,
       const pipeline::DescriptorPool &descriptorPool,
       pipeline::GraphicsPipeline &graphicsPipeline, const Timer &timer,
       pipeline::PipelineMode mode, resource::OffscreenTarget *offscreenTarget)
    : window_(window), instance_(instance), surface_(surface), device_(device),
      command_pool_(commandPool), render_pass_(renderPass),
      descriptor_pool_(descriptorPool), graphics_pipeline_(graphicsPipeline),
      timer_(timer), mode_(mode), offscreen_target_(offscreenTarget) {

  VKR_UI_INFO("Initializing ImGui UI...");
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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
  initInfo.QueueFamily =
      core::QueueFamilyIndices(device_.physicalDevice(), surface_)
          .graphicsFamily();
  initInfo.Queue = device_.graphicsQueue();
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.DescriptorPool = descriptor_pool_.pool();
  initInfo.Allocator = nullptr;
  initInfo.MinImageCount = core::MAX_FRAMES_IN_FLIGHT;
  initInfo.ImageCount = core::MAX_FRAMES_IN_FLIGHT;
  initInfo.CheckVkResultFn = core::check_vk_result;
  initInfo.PipelineInfoMain = pipelineInfo;

  ImGui_ImplVulkan_Init(&initInfo);

  // fps panel
  VKR_UI_INFO("Initializing FPS Panel...");
  fps_panel_ = std::make_unique<FPSPanel>();
  fps_panel_->clear();
  VKR_UI_INFO("FPS Panel initialized successfully.");

  // shader editor
  VKR_UI_INFO("Initializing Shader Editor...");
  shader_editor_ = std::make_unique<ShaderEditor>(
      [this](const std::string &vert, const std::string &frag) {
        std::string err;
        graphics_pipeline_.requestRebuildFromSource(
            vert, frag, [this](bool ok, const std::string &err) {
              shader_editor_->setStatus(ok ? "Compiled successfully." : err,
                                        !ok);
            });
        shader_editor_->setStatus("Compiling...", false);
      });

  shader_editor_->setSource(ShaderType::Vertex,
                            graphics_pipeline_.vertexSource());
  shader_editor_->setSource(ShaderType::Fragment,
                            graphics_pipeline_.fragmentSource());
  shader_editor_->setStatus("Loaded default shaders.", false);
  VKR_UI_INFO("Shader Editor initialized successfully.");

  VKR_UI_INFO("ImGui UI initialized successfully.");
}

UI::~UI() {
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
    renderFullScreen();
    break;
  case LayoutMode::Standard:
    renderDockspace();
    renderMainViewport();
    renderPerformancePanel();
    renderShaderEditor();
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
    if (offscreen_target_ && offscreen_target_->imguiDescriptorSet()) {
      ImGui::Image(reinterpret_cast<ImTextureID>(
                       offscreen_target_->imguiDescriptorSet()),
                   ImGui::GetContentRegionAvail());
    }
  }
  ImGui::End();
  ImGui::PopStyleVar(2);
}

void UI::renderDockspace() {
  static bool dockspaceOpen = true;
  static ImGuiDockNodeFlags dockSpaceFlags = ImGuiDockNodeFlags_None;

  ImGuiWindowFlags windowFlags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

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

  if (dockSpaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
    windowFlags |= ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace", &dockspaceOpen, windowFlags);
  ImGui::PopStyleVar();
  ImGui::PopStyleVar(2);

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit")) {
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      if (ImGui::MenuItem("Full Screen", nullptr,
                          layout_mode_ == LayoutMode::FullScreen))
        layout_mode_ = LayoutMode::FullScreen;
      if (ImGui::MenuItem("Standard Layout", nullptr,
                          layout_mode_ == LayoutMode::Standard))
        layout_mode_ = LayoutMode::Standard;
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

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

void UI::renderMainViewport() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Viewport", nullptr, windowFlags)) {
    ImVec2 panelSize = ImGui::GetContentRegionAvail();
    if (panelSize.x < 1.0f)
      panelSize.x = 1.0f;
    if (panelSize.y < 1.0f)
      panelSize.y = 1.0f;

    if (offscreen_target_ &&
        offscreen_target_->imguiDescriptorSet() != VK_NULL_HANDLE) {
      ImGui::Image(reinterpret_cast<ImTextureID>(
                       offscreen_target_->imguiDescriptorSet()),
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
    viewport_info_.isFocused = ImGui::IsWindowFocused();
    viewport_info_.isHovered = ImGui::IsWindowHovered();
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

void UI::renderPerformancePanel() {
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Performance", nullptr, windowFlags)) {
    if (fps_panel_)
      fps_panel_->render(timer_.fps());
  }
  ImGui::End();
}

void UI::renderShaderEditor() {
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse;
  if (ImGui::Begin("Shader Editor", nullptr, flags))
    shader_editor_->render();
  ImGui::End();
}

void UI::setupDockingLayout() {
  ImGuiID dockSpaceId = ImGui::GetID("DockSpace");
  ImGui::DockBuilderRemoveNode(dockSpaceId);
  ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(dockSpaceId, ImGui::GetMainViewport()->Size);

  ImGuiID dockRight, dockBottomRight, dockShader;
  dockRight = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Right, 0.28f,
                                          nullptr, &dockSpaceId);
  dockShader = ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Up, 0.70f,
                                           nullptr, &dockBottomRight);

  ImGui::DockBuilderDockWindow("Shader Editor", dockShader);
  ImGui::DockBuilderDockWindow("Performance", dockBottomRight);
  ImGui::DockBuilderDockWindow("Viewport", dockSpaceId);
  ImGui::DockBuilderFinish(dockSpaceId);
}

} // namespace vkr::ui
