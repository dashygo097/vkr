#include "vkr/ui/ui.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "vkr/interface/vk_utils.hpp"

UI::UI(GLFWwindow *window, VkInstance instance, VkSurfaceKHR surface,
       VkPhysicalDevice physicalDevice, VkDevice device,
       VkRenderPass renderPass, VkQueue graphicsQueue,
       VkDescriptorPool descriptorPool, VkCommandPool commandPool)
    : window(window), instance(instance), surface(surface),
      physicalDevice(physicalDevice), device(device), renderPass(renderPass),
      graphicsQueue(graphicsQueue), descriptorPool(descriptorPool),
      commandPool(commandPool) {}

UI::UI(const VulkanContext &ctx)
    : UI(ctx.window, ctx.instance, ctx.surface, ctx.physicalDevice, ctx.device,
         ctx.renderPass, ctx.graphicsQueue, ctx.descriptorPool,
         ctx.commandPool) {}

UI::~UI() {}

void UI::render(float fps) { fps_panel->render(fps); }
