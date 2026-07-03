#include "vkr/core/swapchain.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <limits>

namespace vkr::core {

Swapchain::Swapchain(const Window &window, const Surface &surface,
                     const Device &device, SwapchainDesc &desc)
    : window_(window), surface_(surface), device_(device), desc_(desc) {
  VKR_CORE_INFO("Creating initial swapchain...");
  create();
  VKR_CORE_INFO("Initial swapchain created successfully.");
}

Swapchain::~Swapchain() { destroy(); }

void Swapchain::recreate() {
  vkDeviceWaitIdle(device_.device());
  destroy();
  create();
}

void Swapchain::create() {
  SwapchainSupportDetails swapchainSupport =
      querySwapchainSupport(device_.physicalDevice(), surface_.surface());

  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapchainSupport.formats);

  VkPresentModeKHR presentMode = chooseSwapPresentMode(
      swapchainSupport.presentModes, desc_.presentModePolicy);

  VkExtent2D extent =
      chooseSwapExtent(window_.glfwWindow(), swapchainSupport.capabilities);

  uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

  if (swapchainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapchainSupport.capabilities.maxImageCount) {
    imageCount = swapchainSupport.capabilities.maxImageCount;
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
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device_.device(), &createInfo, nullptr,
                           &vk_swapchain_) != VK_SUCCESS) {
    VKR_CORE_ERROR("failed to create swap chain");
  }

  vkGetSwapchainImagesKHR(device_.device(), vk_swapchain_, &imageCount,
                          nullptr);

  vk_color_images_.resize(imageCount);

  vkGetSwapchainImagesKHR(device_.device(), vk_swapchain_, &imageCount,
                          vk_color_images_.data());

  vk_format_ = surfaceFormat.format;
  vk_extent_ = extent;
  vk_present_mode_ = presentMode;

  VKR_CORE_INFO("Swapchain created: extent={}x{}, images={}, format={}, "
                "presentMode={}",
                vk_extent_.width, vk_extent_.height, vk_color_images_.size(),
                static_cast<int>(vk_format_),
                presentModeName(vk_present_mode_));

  vk_color_image_views_.resize(vk_color_images_.size(), VK_NULL_HANDLE);

  for (size_t i = 0; i < vk_color_images_.size(); i++) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = vk_color_images_[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = vk_format_;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_.device(), &viewInfo, nullptr,
                          &vk_color_image_views_[i]) != VK_SUCCESS) {
      VKR_CORE_ERROR("failed to create swapchain image view");
    }
  }
}

void Swapchain::destroy() {
  for (auto imageView : vk_color_image_views_) {
    if (imageView != VK_NULL_HANDLE) {
      vkDestroyImageView(device_.device(), imageView, nullptr);
    }
  }

  vk_color_image_views_.clear();
  vk_color_images_.clear();

  if (vk_swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_.device(), vk_swapchain_, nullptr);
    vk_swapchain_ = VK_NULL_HANDLE;
  }

  vk_format_ = VK_FORMAT_UNDEFINED;
  vk_extent_ = {};
  vk_present_mode_ = VK_PRESENT_MODE_FIFO_KHR;
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
    PresentModePolicy policy) -> VkPresentModeKHR {
  if (availablePresentModes.empty()) {
    VKR_CORE_INFO("No present modes reported. Falling back to FIFO.");
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  for (auto mode : availablePresentModes) {
    VKR_CORE_INFO("Available present mode: {}", presentModeName(mode));
  }

  switch (policy) {
  case PresentModePolicy::Uncapped:
    if (hasPresentMode(availablePresentModes, VK_PRESENT_MODE_IMMEDIATE_KHR)) {
      VKR_CORE_INFO("Selected present mode: IMMEDIATE");
      return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    if (hasPresentMode(availablePresentModes,
                       VK_PRESENT_MODE_FIFO_RELAXED_KHR)) {
      VKR_CORE_INFO("Selected present mode: FIFO_RELAXED fallback");
      return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    }

    if (hasPresentMode(availablePresentModes, VK_PRESENT_MODE_MAILBOX_KHR)) {
      VKR_CORE_INFO("Selected present mode: MAILBOX fallback");
      return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    VKR_CORE_INFO("Selected present mode: FIFO fallback");
    return VK_PRESENT_MODE_FIFO_KHR;

  case PresentModePolicy::Mailbox:
    if (hasPresentMode(availablePresentModes, VK_PRESENT_MODE_MAILBOX_KHR)) {
      VKR_CORE_INFO("Selected present mode: MAILBOX");
      return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    VKR_CORE_INFO("Selected present mode: FIFO fallback");
    return VK_PRESENT_MODE_FIFO_KHR;

  case PresentModePolicy::VSync:
  default:
    VKR_CORE_INFO("Selected present mode: FIFO");
    return VK_PRESENT_MODE_FIFO_KHR;
  }
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

auto querySwapchainSupport(VkPhysicalDevice physicalDevice,
                           VkSurfaceKHR surface) -> SwapchainSupportDetails {
  SwapchainSupportDetails details{};

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                            &details.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                            &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                              &presentModeCount,
                                              details.presentModes.data());
  }

  return details;
}

} // namespace vkr::core
