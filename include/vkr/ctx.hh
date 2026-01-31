#pragma once

#include "./pipeline/graphics_pipeline.hh"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

namespace vkr {

struct VulkanContext {
  // --- App/Engine Info ---
  std::string appName{};
  std::string engineName{};
  uint32_t appVersion{};
  uint32_t engineVersion{};

  // --- Window Info ---
  uint32_t width{};
  uint32_t height{};
  std::string title{"Vulkan Application"};

  // --- Shader paths ---
  std::string vertexShaderPath{};
  std::string fragmentShaderPath{};

  // --- Rendering Mode ---
  pipeline::PipelineMode pipelineMode{pipeline::PipelineMode::Default3D};

  // --- Camera Info ---
  bool cameraEnabled{};
  bool cameraLocked{};
  float cameraMovementSpeed{};
  float cameraMouseSensitivity{};
  float cameraFov{};
  float cameraAspectRatio{};
  float cameraNearPlane{};
  float cameraFarPlane{};

  // --- Runtime ---
  uint32_t currentFrame{};
  bool framebufferResized{};

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
