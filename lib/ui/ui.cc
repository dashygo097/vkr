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
       pipeline::PipelineMode mode)
    : window_(window), instance_(instance), surface_(surface), device_(device),
      command_pool_(commandPool), render_pass_(renderPass),
      descriptor_pool_(descriptorPool), graphics_pipeline_(graphicsPipeline),
      timer_(timer), mode_(mode) {
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
  ImGui_ImplGlfw_InitForVulkan(window.glfwWindow(), true);

  ImGui_ImplVulkan_PipelineInfo pipeline_info = {};
  pipeline_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  pipeline_info.RenderPass = renderPass.renderPass();

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = instance.instance();
  init_info.PhysicalDevice = device.physicalDevice();
  init_info.Device = device.device();
  init_info.QueueFamily =
      core::QueueFamilyIndices(surface, device.physicalDevice())
          .graphicsFamily();
  init_info.Queue = device.graphicsQueue();
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = descriptorPool.pool();
  init_info.Allocator = nullptr;
  init_info.MinImageCount = core::MAX_FRAMES_IN_FLIGHT;
  init_info.ImageCount = core::MAX_FRAMES_IN_FLIGHT;
  init_info.CheckVkResultFn = core::check_vk_result;
  init_info.PipelineInfoMain = pipeline_info;

  ImGui_ImplVulkan_Init(&init_info);

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
    break;

  case LayoutMode::Standard:
    renderDockspace();
    renderMainViewport();
    renderPerformancePanel();
    renderShaderEditor();
    break;
  }

  ImGui::Render();
  ImDrawData *draw_data = ImGui::GetDrawData();
  ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
}

void UI::renderDockspace() {
  static bool dockspaceOpen = true;
  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
  window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
    window_flags |= ImGuiWindowFlags_NoBackground;
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
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
                          layout_mode_ == LayoutMode::FullScreen)) {
        layout_mode_ = LayoutMode::FullScreen;
      }
      if (ImGui::MenuItem("Standard Layout", nullptr,
                          layout_mode_ == LayoutMode::Standard)) {
        layout_mode_ = LayoutMode::Standard;
      }
      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }

  ImGuiIO &io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    static bool first_time = true;
    if (first_time) {
      first_time = false;
      setupDockingLayout();
    }
  }

  ImGui::End();
}

void UI::renderMainViewport() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Viewport", nullptr, window_flags)) {
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

    ImGui::Text("Main Rendering Viewport");
    ImGui::Text("Size: %.0f x %.0f", viewportPanelSize.x, viewportPanelSize.y);

    // ImGui::Image((ImTextureID)m_viewportTextureID, viewportPanelSize);

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
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
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Performance", nullptr, window_flags)) {
    ImGuiIO &io = ImGui::GetIO();

    if (fps_panel_) {
      fps_panel_->render(timer_.fps());
    }
  }
  ImGui::End();
}

void UI::renderShaderEditor() {
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse;
  if (ImGui::Begin("Shader Editor", nullptr, flags)) {
    shader_editor_->render();
  }
  ImGui::End();
}

void UI::setupDockingLayout() {
  ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
  ImGui::DockBuilderRemoveNode(dockspace_id);
  ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

  ImGuiID dock_left, dock_right, dock_bottom_right, dock_shader;

  dock_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f,
                                          nullptr, &dockspace_id);

  dock_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.28f,
                                           nullptr, &dockspace_id);
  dock_shader = ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Up, 0.70f,
                                            nullptr, &dock_bottom_right);

  ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_left);
  ImGui::DockBuilderDockWindow("Shader Editor", dock_shader);
  ImGui::DockBuilderDockWindow("Performance", dock_bottom_right);
  ImGui::DockBuilderDockWindow("Viewport", dockspace_id);

  ImGui::DockBuilderFinish(dockspace_id);
}

} // namespace vkr::ui
