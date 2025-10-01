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

  ImGui::StyleColorsDark();

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

  // Main Settings Panel
  ImGui::SetNextWindowPos(ImVec2(padding, padding), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(240, 0), ImGuiCond_Always);

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("Engine Controller", nullptr, window_flags)) {
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

  // Performance Panel
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 260 - padding, padding),
                          ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(260, 0), ImGuiCond_Always);

  ImGuiWindowFlags performance_flags = ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoCollapse;

  // Performance Overlay Panel
  if (ImGui::Begin("Performance", nullptr, performance_flags)) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.00f, 0.00f, 0.00f, 0.20f));
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

      ImGui::TextColored(ImVec4(0.70f, 1.00f, 0.85f, 1.0f), "Current: %.1f FPS",
                         fps);
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

    fps_panel->render(fps);
  }

  ImGui::End();

  ImGui::Render();
  ImDrawData *draw_data = ImGui::GetDrawData();
  ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
}

} // namespace vkr
