#include "vkr/core/swapchain.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <limits>
#include <vector>

namespace vkr::core {
namespace {

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
  return availableFormats.front();
}

auto chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes,
    VkPresentModeKHR requestedMode) -> VkPresentModeKHR {
  if (std::find(availablePresentModes.begin(), availablePresentModes.end(),
                requestedMode) != availablePresentModes.end()) {
    return requestedMode;
  }

  VKR_CORE_WARN("requested present mode not found, falling back to FIFO");
  return VK_PRESENT_MODE_FIFO_KHR;
}

auto chooseSwapExtent(const Window &window,
                      const VkSurfaceCapabilitiesKHR &capabilities)
    -> VkExtent2D {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window.glfwWindow(), &width, &height);
  return {
      std::clamp(static_cast<uint32_t>(std::max(width, 0)),
                 capabilities.minImageExtent.width,
                 capabilities.maxImageExtent.width),
      std::clamp(static_cast<uint32_t>(std::max(height, 0)),
                 capabilities.minImageExtent.height,
                 capabilities.maxImageExtent.height),
  };
}

} // namespace

Swapchain::Swapchain(const Window &window, const Surface &surface,
                     const Device &device, const SwapchainDesc &desc)
    : window_(window), surface_(surface), device_(device), desc_(desc) {
  create();
}

Swapchain::~Swapchain() { destroy(); }

void Swapchain::update(const SwapchainDesc &desc) {
  if (!desc.isValid()) {
    VKR_CORE_ERROR("Invalid swapchain descriptor");
  }
  desc_ = desc;
  recreate();
}

void Swapchain::recreate() {
  VKR_CORE_INFO("Recreating swapchain...");
  destroy();
  create();
}

void Swapchain::create() {
  if (!desc_.isValid()) {
    VKR_CORE_ERROR("Invalid swapchain descriptor");
  }
  VKR_CORE_INFO("Creating swapchain...");

  VkSurfaceCapabilitiesKHR capabilities{};
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_.physicalDevice(),
                                                surface_.surface(),
                                                &capabilities) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to query surface capabilities");
  }

  VkResult result{};
  uint32_t formatCount = 0;
  std::vector<VkSurfaceFormatKHR> formats{};
  do {
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(device_.physicalDevice(),
                                             surface_.surface(), &formatCount,
                                             nullptr) != VK_SUCCESS) {
      VKR_CORE_ERROR("Failed to query surface format count");
    }
    if (formatCount == 0) {
      VKR_CORE_ERROR("Surface has no swapchain formats");
    }
    formats.resize(formatCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(device_.physicalDevice(),
                                                  surface_.surface(),
                                                  &formatCount, formats.data());
  } while (result == VK_INCOMPLETE);
  if (result != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to query surface formats");
  }
  formats.resize(formatCount);

  uint32_t presentModeCount = 0;
  std::vector<VkPresentModeKHR> presentModes{};
  do {
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(
            device_.physicalDevice(), surface_.surface(), &presentModeCount,
            nullptr) != VK_SUCCESS) {
      VKR_CORE_ERROR("Failed to query surface present mode count");
    }
    if (presentModeCount == 0) {
      VKR_CORE_ERROR("Surface has no swapchain present modes");
    }
    presentModes.resize(presentModeCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        device_.physicalDevice(), surface_.surface(), &presentModeCount,
        presentModes.data());
  } while (result == VK_INCOMPLETE);
  if (result != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to query surface present modes");
  }
  presentModes.resize(presentModeCount);

  const auto surfaceFormat = chooseSwapSurfaceFormat(formats);
  const auto presentMode =
      chooseSwapPresentMode(presentModes, desc_.presentMode);
  const auto extent = chooseSwapExtent(window_, capabilities);
  if (extent.width == 0 || extent.height == 0) {
    VKR_CORE_ERROR("Cannot create a swapchain for a zero-sized framebuffer");
  }

  uint32_t imageCount =
      std::max(desc_.minImageCount, capabilities.minImageCount);
  if (desc_.minImageCount == 0 &&
      imageCount < std::numeric_limits<uint32_t>::max()) {
    ++imageCount;
  }
  if (capabilities.maxImageCount != 0 &&
      imageCount > capabilities.maxImageCount) {
    if (desc_.minImageCount != 0) {
      VKR_CORE_ERROR(
          "Requested minimum image count {} exceeds surface maximum {}",
          desc_.minImageCount, capabilities.maxImageCount);
    }
    imageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface_.surface();
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
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
  }

  createInfo.preTransform = capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  VkSwapchainKHR swapchain{VK_NULL_HANDLE};
  if (vkCreateSwapchainKHR(device_.device(), &createInfo, nullptr,
                           &swapchain) != VK_SUCCESS) {
    VKR_CORE_ERROR("Failed to create swapchain");
  }

  uint32_t actualImageCount = 0;
  if (vkGetSwapchainImagesKHR(device_.device(), swapchain, &actualImageCount,
                              nullptr) != VK_SUCCESS ||
      actualImageCount == 0) {
    vkDestroySwapchainKHR(device_.device(), swapchain, nullptr);
    VKR_CORE_ERROR("Failed to query swapchain images");
  }

  vk_swapchain_ = swapchain;
  surface_format_ = surfaceFormat;
  extent_ = extent;
  image_count_ = actualImageCount;
  present_mode_ = presentMode;

  VKR_CORE_INFO(
      "Swapchain created: extent={}x{}, images={}, format={}, presentMode={}",
      extent_.width, extent_.height, image_count_,
      static_cast<int>(surface_format_.format),
      static_cast<int>(present_mode_));
}

void Swapchain::destroy() {
  if (vk_swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_.device(), vk_swapchain_, nullptr);
    vk_swapchain_ = VK_NULL_HANDLE;
  }
  surface_format_ = {};
  extent_ = {};
  image_count_ = 0;
  present_mode_ = VK_PRESENT_MODE_MAX_ENUM_KHR;
}

} // namespace vkr::core
