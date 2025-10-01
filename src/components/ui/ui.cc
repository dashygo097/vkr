#include "vkr/components/ui/ui.hh"
#include "vkr/interface/vk_utils.hh"

namespace vkr {
static void check_vk_result(VkResult err) {
  if (err == 0)
    return;
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if (err < 0)
    abort();
}

static void applyDefaultStyle() {
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 *colors = style.Colors;

  colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.75f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.25f);
  colors[ImGuiCol_Border] = ImVec4(1.00f, 1.00f, 1.00f, 0.15f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.40f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.30f, 0.60f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.05f, 0.05f, 0.05f, 0.65f);
  colors[ImGuiCol_Button] = ImVec4(0.90f, 0.90f, 0.90f, 0.20f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.35f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.80f, 0.80f, 0.50f);
  colors[ImGuiCol_Header] = ImVec4(0.90f, 0.90f, 0.90f, 0.15f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.95f, 0.95f, 0.95f, 0.35f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.95f, 0.95f, 0.95f, 0.80f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
  colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 0.60f);
  colors[ImGuiCol_Separator] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.15f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.80f, 0.30f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.90f, 0.90f, 0.90f, 0.40f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);

  style.WindowRounding = 12.0f;
  style.FrameRounding = 6.0f;
  style.GrabRounding = 6.0f;
  style.ChildRounding = 8.0f;
  style.ScrollbarRounding = 12.0f;
  style.WindowPadding = ImVec2(16.0f, 16.0f);
  style.FramePadding = ImVec2(10.0f, 5.0f);
  style.ItemSpacing = ImVec2(10.0f, 8.0f);
  style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
}

UI::UI(GLFWwindow *window, VkInstance instance, VkSurfaceKHR surface,
       VkPhysicalDevice physicalDevice, VkDevice device,
       VkRenderPass renderPass, VkQueue graphicsQueue,
       VkDescriptorPool descriptorPool, VkCommandPool commandPool)
    : window(window), instance(instance), surface(surface),
      physicalDevice(physicalDevice), device(device), renderPass(renderPass),
      graphicsQueue(graphicsQueue), descriptorPool(descriptorPool),
      commandPool(commandPool) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  applyDefaultStyle();

  ImGui_ImplGlfw_InitForVulkan(window, true);

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = instance;
  init_info.PhysicalDevice = physicalDevice;
  init_info.Device = device;
  init_info.QueueFamily =
      findQueueFamilies(physicalDevice, surface).graphicsFamily.value();
  init_info.Queue = graphicsQueue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = descriptorPool;
  init_info.Allocator = nullptr;
  init_info.MinImageCount = 2;
  init_info.ImageCount = 2;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.CheckVkResultFn = check_vk_result;
  init_info.RenderPass = renderPass;

  ImGui_ImplVulkan_Init(&init_info);
}

UI::UI(const VulkanContext &ctx)
    : UI(ctx.window, ctx.instance, ctx.surface, ctx.physicalDevice, ctx.device,
         ctx.renderPass, ctx.graphicsQueue, ctx.descriptorPool,
         ctx.commandPool) {}
UI::~UI() {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void UI::render(VkCommandBuffer commandBuffer) {
  if (!_visible)
    return;

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGuiIO &io = ImGui::GetIO();
  const float padding = 16.0f;

  ImGui::SetNextWindowPos(ImVec2(padding, padding), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(240, 0), ImGuiCond_Always);

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Vulkan Renderer", nullptr, window_flags)) {
    ImGui::PushFont(io.Fonts->Fonts[0]);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.95f));
    ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.95f, 0.90f), "ENGINE CONTROLLER");
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Performance",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::PushStyleColor(ImGuiCol_ChildBg,
                            ImVec4(0.00f, 0.00f, 0.00f, 0.20f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.10f));
      ImGui::BeginChild("PerformanceMetrics", ImVec2(0, 120), true,
                        ImGuiWindowFlags_NoScrollbar);

      float fps = io.Framerate;
      ImVec4 fps_color = fps > 60   ? ImVec4(0.60f, 1.00f, 0.80f, 0.85f)
                         : fps > 30 ? ImVec4(1.00f, 0.95f, 0.55f, 0.85f)
                                    : ImVec4(1.00f, 0.60f, 0.60f, 0.85f);

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
      ImGui::TextColored(fps_color, "FPS: %.1f", fps);
      ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 0.80f),
                         "Frame Time: %.2f ms", 1000.0f / fps);
      ImGui::PopStyleVar();

      if (ImGui::IsItemHovered() || ImGui::IsWindowHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));

        ImGui::Text("Performance Metrics");
        ImGui::Separator();
        ImGui::Spacing();

        static float min_fps = fps;
        static float max_fps = fps;
        static float avg_sum = 0.0f;
        static int avg_count = 0;

        min_fps = (fps < min_fps) ? fps : min_fps;
        max_fps = (fps > max_fps) ? fps : max_fps;
        avg_sum += fps;
        avg_count++;
        float avg_fps = avg_sum / avg_count;

        static float reset_timer = 0.0f;
        reset_timer += io.DeltaTime;
        if (reset_timer > 5.0f) {
          min_fps = fps;
          max_fps = fps;
          avg_sum = fps;
          avg_count = 1;
          reset_timer = 0.0f;
        }

        ImGui::TextColored(ImVec4(0.70f, 1.00f, 0.85f, 1.0f),
                           "Current: %.1f FPS", fps);
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 0.90f),
                           "Average: %.1f FPS", avg_fps);
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 0.90f), "Min: %.1f FPS",
                           min_fps);
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 0.90f), "Max: %.1f FPS",
                           max_fps);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 0.80f),
                           "Frame Time: %.3f ms", 1000.0f / fps);
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 0.80f),
                           "Delta Time: %.3f ms", io.DeltaTime * 1000.0f);

        ImGui::PopStyleColor();
        ImGui::EndTooltip();
      }
      ImGui::Spacing();

      static float fps_history[90] = {};
      static int fps_index = 0;
      fps_history[fps_index] = fps;
      fps_index = (fps_index + 1) % 90;

      ImGui::PushStyleColor(ImGuiCol_FrameBg,
                            ImVec4(0.05f, 0.05f, 0.05f, 0.30f));
      ImGui::PushStyleColor(ImGuiCol_PlotLines,
                            ImVec4(0.70f, 1.00f, 0.85f, 0.70f));
      ImGui::PlotLines("##FPSGraph", fps_history, 90, fps_index, nullptr, 0.0f,
                       120.0f, ImVec2(0, 45));
      ImGui::PopStyleColor(2);

      ImGui::EndChild();
      ImGui::PopStyleColor(2);
    }

    ImGui::Spacing();

    // Rendering section
    if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
      // Placeholder for rendering controls
    }

    ImGui::Spacing();

    // Scene section
    if (ImGui::CollapsingHeader("Scene")) {
      // Placeholder for scene controls
    }

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 1.0f, 0.12f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.95f, 0.95f, 0.95f, 0.18f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(1.00f, 1.00f, 1.00f, 0.28f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.85f, 0.85f, 0.85f, 0.40f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));

    if (ImGui::Button("Take Screenshot", ImVec2(-1, 32))) {
      // Placeholder for screenshot logic
    }

    ImGui::PopStyleColor(4);
  }
  ImGui::End();

  // FPS panel
  if (fps_panel) {
    ImGui::SetNextWindowPos(ImVec2(padding, io.DisplaySize.y - 70 - padding),
                            ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 70), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.60f);

    ImGuiWindowFlags overlay_flags = ImGuiWindowFlags_NoDecoration |
                                     ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_WindowBg,
                          ImVec4(0.03f, 0.03f, 0.03f, 0.65f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.12f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    if (ImGui::Begin("FPS Overlay", nullptr, overlay_flags)) {
      fps_panel->render();
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
  }

  ImGui::Render();
  ImDrawData *draw_data = ImGui::GetDrawData();
  ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
}

} // namespace vkr
