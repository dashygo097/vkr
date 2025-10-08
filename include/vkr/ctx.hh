#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

namespace vkr {

struct VulkanContext {

  // --- App/Engine Info ---
  std::string appName;
  std::string engineName;
  uint32_t appVersion;
  uint32_t engineVersion;

  // --- Window Info ---
  uint32_t width;
  uint32_t height;
  std::string title;

  // --- Camera Info ---
  bool cameraEnabled;
  bool cameraLocked;
  float cameraMovementSpeed;
  float cameraMouseSensitivity;
  float cameraFov;
  float cameraAspectRatio;
  float cameraNearPlane;
  float cameraFarPlane;

  // --- Shader paths ---
  std::string vertexShaderPath;
  std::string fragmentShaderPath;

  // --- Vulkan Core Objects ---
  GLFWwindow *window = nullptr;
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
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline graphicsPipeline = VK_NULL_HANDLE;

  // --- CommandPool ---
  VkCommandPool commandPool = VK_NULL_HANDLE;

  // --- Uniform Buffers ---
  std::vector<VkBuffer> uniformBuffers = {};
  std::vector<VkDeviceMemory> uniformBuffersMemory = {};
  std::vector<void *> uniformBuffersMapped = {};

  // --- Descriptor Sets ---
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets = {};

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
} // namespace vkr
