#include "vkr/components/ui/ui.hpp"
#include "vkr/interface/vk_utils.hpp"

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

  ImGui::StyleColorsLight();

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
  ImGui_ImplVulkan_NewFrame();

  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("Hello, Vulkan!");
  ImGui::Text("This is a simple ImGui window.");
  ImGui::End();

  fps_panel->render();

  ImGui::Render();
  ImDrawData *draw_data = ImGui::GetDrawData();

  ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
}
} // namespace vkr
