#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

struct VulkanContext {
  // --- App/Engine Info ---
  std::string appName = "VulkanApp";
  std::string engineName = "No Engine";
  uint32_t appVersion = VK_MAKE_VERSION(1, 0, 0);
  uint32_t engineVersion = VK_MAKE_VERSION(1, 0, 0);

  // --- Window Info ---
  uint32_t width = 800;
  uint32_t height = 600;
  std::string title = "Vulkan (Default Title)";
  GLFWwindow *window = nullptr;

  // --- Shader paths ---
  std::string vertexShaderPath = "shaders/vert.spv";
  std::string fragmentShaderPath = "shaders/frag.spv";

  // --- Vulkan Core Objects ---
  VkInstance instance = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;

  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentQueue = VK_NULL_HANDLE;

  // --- Swapchain ---
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  std::vector<VkImage> swapchainImages = {};
  std::vector<VkImageView> swapchainImageViews = {};
  VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D swapchainExtent = {};
  std::vector<VkFramebuffer> swapchainFramebuffers = {};

  // --- Rendering ---
  VkRenderPass renderPass = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline graphicsPipeline = VK_NULL_HANDLE;

  // --- CommandPool ---
  VkCommandPool commandPool = VK_NULL_HANDLE;

  // --- Vertex Buffers ---
  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

  // --- Index Buffers ---
  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

  // --- Command Buffers ---
  std::vector<VkCommandBuffer> commandBuffers = {};

  // --- Synchronization ---
  std::vector<VkSemaphore> imageAvailableSemaphores = {};
  std::vector<VkSemaphore> renderFinishedSemaphores = {};
  std::vector<VkFence> inFlightFences = {};

  // --- Runtime ---
  uint32_t currentFrame = 0;
  bool framebufferResized = false;

  // --- Validation & Extensions ---
  std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
  std::vector<const char *> preExtensions = {
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
  };
  std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
  };

  // --- Helpers ---
  bool isValidationEnabled() const {
#ifdef NDEBUG
    return false;
#else
    return !validationLayers.empty();
#endif
  }
};
