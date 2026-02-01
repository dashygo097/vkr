#include "vkr/ui/ui.hh"
#include "vkr/core/core_utils.hh"
#include "vkr/core/queue_families.hh"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

namespace vkr::ui {
UI::UI(const core::Window &window, const core::Instance &instance,
       const core::Surface &surface, const core::Device &device,
       const core::CommandPool &commandPool,
       const pipeline::RenderPass &renderPass,
       const pipeline::DescriptorPool &descriptorPool)
    : window_(window), instance_(instance), surface_(surface), device_(device),
      render_pass_(renderPass), descriptor_pool_(descriptorPool),
      command_pool_(commandPool) {
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
  init_info.MinImageCount = 2;
  init_info.ImageCount = 2;
  init_info.CheckVkResultFn = core::check_vk_result;
  init_info.PipelineInfoMain = pipeline_info;

  ImGui_ImplVulkan_Init(&init_info);
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

    float fps = io.Framerate;
    ImVec4 fps_color = fps > 60   ? ImVec4(0.60f, 1.00f, 0.80f, 0.85f)
                       : fps > 30 ? ImVec4(1.00f, 0.95f, 0.55f, 0.85f)
                                  : ImVec4(1.00f, 0.60f, 0.60f, 0.85f);

    ImGui::TextColored(fps_color, "FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.2f ms", 1000.0f / fps);

    ImGui::Separator();

    if (fps_panel_) {
      fps_panel_->render(fps);
    }
  }
  ImGui::End();
}

void UI::setupDockingLayout() {
  ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
  ImGui::DockBuilderRemoveNode(dockspace_id);
  ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

  ImGuiID dock_left, dock_right, dock_bottom_right;

  dock_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f,
                                          nullptr, &dockspace_id);

  dock_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f,
                                           nullptr, &dockspace_id);

  dock_bottom_right = ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down,
                                                  0.3f, nullptr, &dock_right);

  ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_left);
  ImGui::DockBuilderDockWindow("Performance", dock_bottom_right);
  ImGui::DockBuilderDockWindow("Viewport", dockspace_id);

  ImGui::DockBuilderFinish(dockspace_id);
}

} // namespace vkr::ui
