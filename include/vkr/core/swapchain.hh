#pragma once

#include "vkr/core/device.hh"
#include "vkr/core/surface.hh"
#include "vkr/core/window.hh"

namespace vkr::core {

enum class PresentModePolicy {
  VSync,
  Mailbox,
  Uncapped,
};

struct SwapchainDesc {
  PresentModePolicy presentModePolicy{PresentModePolicy::Uncapped};

  [[nodiscard]] auto isValid() const noexcept -> bool { return true; }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("present", presentModePolicy);
  }
};

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities{};
  std::vector<VkSurfaceFormatKHR> formats{};
  std::vector<VkPresentModeKHR> presentModes{};
};

auto querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    -> SwapchainSupportDetails;

auto chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats)
    -> VkSurfaceFormatKHR;

auto chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes,
    PresentModePolicy policy) -> VkPresentModeKHR;

auto chooseSwapExtent(GLFWwindow *window,
                      const VkSurfaceCapabilitiesKHR &capabilities)
    -> VkExtent2D;

auto presentModeName(VkPresentModeKHR mode) -> const char *;

class Swapchain {
public:
  explicit Swapchain(const Window &window, const Surface &surface,
                     const Device &device, SwapchainDesc &desc);
  ~Swapchain();

  Swapchain(const Swapchain &) = delete;
  auto operator=(const Swapchain &) -> Swapchain & = delete;

  [[nodiscard]] auto desc() const noexcept -> const SwapchainDesc & {
    return desc_;
  }

  void create();
  void destroy();
  void recreate();

  [[nodiscard]] auto swapchain() const noexcept -> VkSwapchainKHR {
    return vk_swapchain_;
  }

  [[nodiscard]] auto images() const noexcept -> const std::vector<VkImage> & {
    return vk_color_images_;
  }

  [[nodiscard]] auto image(size_t index) const -> VkImage {
    return vk_color_images_.at(index);
  }

  [[nodiscard]] auto imageViews() const noexcept
      -> const std::vector<VkImageView> & {
    return vk_color_image_views_;
  }

  [[nodiscard]] auto imageView(size_t index) const -> VkImageView {
    return vk_color_image_views_.at(index);
  }

  [[nodiscard]] auto imageCount() const noexcept -> size_t {
    return vk_color_images_.size();
  }

  [[nodiscard]] auto format() const noexcept -> VkFormat { return vk_format_; }

  [[nodiscard]] auto extent2D() const noexcept -> VkExtent2D {
    return vk_extent_;
  }

private:
  // dependencies
  const Window &window_;
  const Surface &surface_;
  const Device &device_;

  // components
  SwapchainDesc &desc_;
  VkSwapchainKHR vk_swapchain_{VK_NULL_HANDLE};
  std::vector<VkImage> vk_color_images_{};
  std::vector<VkImageView> vk_color_image_views_{};
  VkFormat vk_format_{VK_FORMAT_UNDEFINED};
  VkExtent2D vk_extent_{};
  VkPresentModeKHR vk_present_mode_{VK_PRESENT_MODE_FIFO_KHR};
};

} // namespace vkr::core
