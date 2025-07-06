#include "vkr/ui/ui.hpp"

/*

ImGui_ImplGlfw_InitForVulkan(window, true);  // If using GLFW
ImGui_ImplVulkan_InitInfo init_info = {};
init_info.Instance = ctx.instance;
init_info.PhysicalDevice = ctx.physicalDevice;
init_info.Device = ctx.device;
init_info.QueueFamily = graphicsQueueFamilyIndex;
init_info.Queue = ctx.graphicsQueue;
init_info.PipelineCache = VK_NULL_HANDLE;
init_info.DescriptorPool = your_imgui_descriptor_pool;
init_info.Subpass = 0;
init_info.MinImageCount = min_image_count;
init_info.ImageCount = swapchain_image_count;
init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
init_info.Allocator = nullptr;
init_info.CheckVkResultFn = check_vk_result;  // your error check callback
ImGui_ImplVulkan_Init(&init_info, ctx.renderPass);

*/

UI::UI(GLFWwindow *window, VkInstance instance, VkPhysicalDevice physicalDevice,
       VkDevice device, VkRenderPass renderPass, VkQueue graphicsQueue)
    : window(window), instance(instance), physicalDevice(physicalDevice),
      device(device), renderPass(renderPass), graphicsQueue(graphicsQueue) {
  fps_panel = std::make_unique<FPSPanel>();
}

UI::UI(const VulkanContext &ctx)
    : UI(ctx.window, ctx.instance, ctx.physicalDevice, ctx.device,
         ctx.renderPass, ctx.graphicsQueue) {}

void UI::draw(float fps) { fps_panel->draw(fps); }
