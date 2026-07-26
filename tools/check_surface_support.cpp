#include "vk_tool_common.hh"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

auto presentModeString(VkPresentModeKHR mode) -> const char * {
  switch (mode) {
  case VK_PRESENT_MODE_IMMEDIATE_KHR:
    return "immediate";
  case VK_PRESENT_MODE_MAILBOX_KHR:
    return "mailbox";
  case VK_PRESENT_MODE_FIFO_KHR:
    return "fifo";
  case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
    return "fifo-relaxed";
  default:
    return "unknown";
  }
}

auto colorSpaceString(VkColorSpaceKHR colorSpace) -> const char * {
  switch (colorSpace) {
  case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
    return "srgb-nonlinear";
  case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
    return "display-p3-nonlinear";
  case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
    return "extended-srgb-linear";
  case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:
    return "display-p3-linear";
  case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:
    return "dci-p3-nonlinear";
  case VK_COLOR_SPACE_BT709_LINEAR_EXT:
    return "bt709-linear";
  case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:
    return "bt709-nonlinear";
  case VK_COLOR_SPACE_HDR10_ST2084_EXT:
    return "hdr10-st2084";
  case VK_COLOR_SPACE_DOLBYVISION_EXT:
    return "dolbyvision";
  case VK_COLOR_SPACE_HDR10_HLG_EXT:
    return "hdr10-hlg";
  case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
    return "adobergb-linear";
  case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:
    return "adobergb-nonlinear";
  case VK_COLOR_SPACE_PASS_THROUGH_EXT:
    return "pass-through";
  case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:
    return "extended-srgb-nonlinear";
  default:
    return "unknown";
  }
}

auto extentString(VkExtent2D extent) -> std::string {
  return std::to_string(extent.width) + "x" + std::to_string(extent.height);
}

} // namespace

auto main() -> int {
  try {
    if (glfwInit() != GLFW_TRUE) {
      throw std::runtime_error("failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *window =
        glfwCreateWindow(640, 480, "check_surface_support", nullptr, nullptr);
    if (window == nullptr) {
      glfwTerminate();
      throw std::runtime_error("failed to create GLFW window");
    }

    VkInstance instance = vktool::createInstance("check_surface_support", true);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
        VK_SUCCESS) {
      vkDestroyInstance(instance, nullptr);
      glfwDestroyWindow(window);
      glfwTerminate();
      throw std::runtime_error("failed to create window surface");
    }

    const auto devices = vktool::physicalDevices(instance);

    std::cout << "Surface Support:\n";
    std::cout << "================\n";

    for (uint32_t deviceIndex = 0; deviceIndex < devices.size();
         ++deviceIndex) {
      VkPhysicalDeviceProperties props{};
      vkGetPhysicalDeviceProperties(devices[deviceIndex], &props);

      std::cout << "GPU" << deviceIndex << ": " << props.deviceName << "\n";

      uint32_t familyCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex],
                                               &familyCount, nullptr);
      std::vector<VkQueueFamilyProperties> families(familyCount);
      vkGetPhysicalDeviceQueueFamilyProperties(
          devices[deviceIndex], &familyCount, families.data());

      std::cout << "        present support per queue family:\n";
      for (uint32_t familyIndex = 0; familyIndex < familyCount;
           ++familyIndex) {
        VkBool32 presentSupported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(devices[deviceIndex], familyIndex,
                                             surface, &presentSupported);
        std::cout << "          [" << familyIndex
                  << "] present = "
                  << (presentSupported == VK_TRUE ? "true" : "false")
                  << ", flags ="
                  << vktool::queueFlagsString(families[familyIndex].queueFlags)
                  << "\n";
      }

      VkSurfaceCapabilitiesKHR capabilities{};
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex], surface,
                                                &capabilities);

      std::cout << "        minImageCount      = "
                << capabilities.minImageCount << "\n";
      std::cout << "        maxImageCount      = "
                << capabilities.maxImageCount << "\n";
      std::cout << "        currentExtent      = "
                << extentString(capabilities.currentExtent) << "\n";
      std::cout << "        minImageExtent     = "
                << extentString(capabilities.minImageExtent) << "\n";
      std::cout << "        maxImageExtent     = "
                << extentString(capabilities.maxImageExtent) << "\n";
      std::cout << "        currentTransform   = 0x" << std::hex
                << capabilities.currentTransform << std::dec << "\n";
      std::cout << "        supportedTransforms= 0x" << std::hex
                << capabilities.supportedTransforms << std::dec << "\n";
      std::cout << "        compositeAlpha     = 0x" << std::hex
                << capabilities.supportedCompositeAlpha << std::dec << "\n";
      std::cout << "        supportedUsage     = 0x" << std::hex
                << capabilities.supportedUsageFlags << std::dec << "\n";

      uint32_t formatCount = 0;
      vkGetPhysicalDeviceSurfaceFormatsKHR(devices[deviceIndex], surface,
                                           &formatCount, nullptr);
      std::vector<VkSurfaceFormatKHR> formats(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(devices[deviceIndex], surface,
                                           &formatCount, formats.data());

      std::cout << "        surfaceFormats = " << formats.size() << "\n";
      for (const auto &format : formats) {
        std::cout << "          format = " << format.format
                  << ", colorSpace = "
                  << colorSpaceString(format.colorSpace) << "\n";
      }

      uint32_t presentModeCount = 0;
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          devices[deviceIndex], surface, &presentModeCount, nullptr);
      std::vector<VkPresentModeKHR> presentModes(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          devices[deviceIndex], surface, &presentModeCount,
          presentModes.data());

      std::cout << "        presentModes = " << presentModes.size() << "\n";
      for (const auto mode : presentModes) {
        std::cout << "          " << presentModeString(mode) << "\n";
      }
      std::cout << "\n";
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
  } catch (const std::exception &e) {
    std::cerr << "check_surface_support failed: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
