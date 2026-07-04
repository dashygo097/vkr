#include "vkr/core/swapchain.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <limits>

namespace vkr::core {

Swapchain::Swapchain(const Window &window, const Surface &surface,
                     const Device &device, SwapchainDesc &desc)
    : window_(window), surface_(surface), device_(device), desc_(desc) {
  VKR_CORE_INFO("Creating initial swapchain...");

  querySwapchainSupport(device_.physicalDevice(), surface_.surface(), desc_);

  desc_.surfaceFormat = chooseSwapSurfaceFormat(desc_.formats);
  desc_.presentMode =
      chooseSwapPresentMode(desc_.presentModes, desc_.presentMode);
  desc_.extent = chooseSwapExtent(window_.glfwWindow(), desc_.capabilities);

  uint32_t imageCount = desc_.capabilities.minImageCount + 1;

  if (desc_.capabilities.maxImageCount > 0 &&
      imageCount > desc_.capabilities.maxImageCount) {
    imageCount = desc_.capabilities.maxImageCount;
  }

  desc_.imageCount = imageCount;

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface_.surface();
  createInfo.minImageCount = desc_.imageCount;
  createInfo.imageFormat = desc_.surfaceFormat.format;
  createInfo.imageColorSpace = desc_.surfaceFormat.colorSpace;
  createInfo.imageExtent = desc_.extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = {device_.graphicsFamily(),
                                   device_.presentFamily()};

  if (device_.graphicsFamily() != device_.presentFamily()) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  createInfo.preTransform = desc_.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = desc_.presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device_.device(), &createInfo, nullptr,
                           &vk_swapchain_) != VK_SUCCESS) {
    VKR_CORE_ERROR("failed to create swap chain");
  }

  uint32_t actualImageCount = 0;
  vkGetSwapchainImagesKHR(device_.device(), vk_swapchain_, &actualImageCount,
                          nullptr);
  desc_.imageCount = actualImageCount;

  VKR_CORE_INFO("Swapchain created: extent={}x{}, images={}, format={}, "
                "presentMode={}",
                desc_.extent.width, desc_.extent.height, desc_.imageCount,
                static_cast<int>(desc_.surfaceFormat.format),
                presentModeName(desc_.presentMode));

  VKR_CORE_INFO("Initial swapchain created successfully.");
}

Swapchain::~Swapchain() {
  if (vk_swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_.device(), vk_swapchain_, nullptr);
    vk_swapchain_ = VK_NULL_HANDLE;
  }

  desc_.extent = {};
  desc_.imageCount = 0;
  desc_.surfaceFormat = {};
}

auto chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats)
    -> VkSurfaceFormatKHR {
  if (availableFormats.empty()) {
    VKR_CORE_ERROR("no available swapchain surface formats");
  }

  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

auto presentModeName(VkPresentModeKHR mode) -> const char * {
  switch (mode) {
  case VK_PRESENT_MODE_IMMEDIATE_KHR:
    return "IMMEDIATE";
  case VK_PRESENT_MODE_MAILBOX_KHR:
    return "MAILBOX";
  case VK_PRESENT_MODE_FIFO_KHR:
    return "FIFO";
  case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
    return "FIFO_RELAXED";
  default:
    return "UNKNOWN";
  }
}

auto hasPresentMode(const std::vector<VkPresentModeKHR> &modes,
                    VkPresentModeKHR target) -> bool {
  return std::find(modes.begin(), modes.end(), target) != modes.end();
}

auto chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes,
    VkPresentModeKHR requestedMode) -> VkPresentModeKHR {
  if (availablePresentModes.empty()) {
    VKR_CORE_INFO("No present modes reported. Falling back to FIFO.");
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  for (auto mode : availablePresentModes) {
    VKR_CORE_INFO("Available present mode: {}", presentModeName(mode));
  }

  if (hasPresentMode(availablePresentModes, requestedMode)) {
    VKR_CORE_INFO("Selected present mode: {}", presentModeName(requestedMode));
    return requestedMode;
  }

  VKR_CORE_INFO("Requested present mode {} unavailable. Falling back to FIFO.",
                presentModeName(requestedMode));
  return VK_PRESENT_MODE_FIFO_KHR;
}

auto chooseSwapExtent(GLFWwindow *window,
                      const VkSurfaceCapabilitiesKHR &capabilities)
    -> VkExtent2D {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);

  VkExtent2D actualExtent = {
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height),
  };

  actualExtent.width =
      std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                 capabilities.maxImageExtent.width);

  actualExtent.height =
      std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                 capabilities.maxImageExtent.height);

  return actualExtent;
}

void querySwapchainSupport(VkPhysicalDevice physicalDevice,
                           VkSurfaceKHR surface, SwapchainDesc &desc) {
  desc.capabilities = {};
  desc.formats.clear();
  desc.presentModes.clear();

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                            &desc.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       nullptr);

  if (formatCount != 0) {
    desc.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                         desc.formats.data());
  }

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                            &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    desc.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice, surface, &presentModeCount, desc.presentModes.data());
  }
}

} // namespace vkr::core
