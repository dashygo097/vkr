#pragma once

#include "../../core/command_pool.hh"
#include "../../core/device.hh"
#include <stb_image.h>

namespace vkr::resource {
class Image {
public:
  explicit Image(const core::Device &device,
                 const core::CommandPool &commandPool);
  ~Image();

  Image(const Image &) = delete;
  auto operator=(const Image &) -> Image & = delete;

  void create(uint32_t width, uint32_t height, VkFormat format,
              VkImageTiling tiling, VkImageUsageFlags usage,
              VkMemoryPropertyFlags properties);
  void create(const std::string &imageFilePath);
  void destroy();
  void update(const std::string &imageFilePath);

  [[nodiscard]] auto width() const -> uint32_t {
    return static_cast<uint32_t>(width_);
  }
  [[nodiscard]] auto height() const -> uint32_t {
    return static_cast<uint32_t>(height_);
  }
  [[nodiscard]] auto channels() const -> uint32_t {
    return static_cast<uint32_t>(channels_);
  }
  [[nodiscard]] auto image() const -> VkImage { return vk_image_; }
  [[nodiscard]] auto memory() const -> VkDeviceMemory { return vk_memory_; }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  int width_{0};
  int height_{0};
  int channels_{0};
  VkImage vk_image_{VK_NULL_HANDLE};
  VkDeviceMemory vk_memory_{VK_NULL_HANDLE};

  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout);

  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height);

  auto beginSingleTimeCommands() -> VkCommandBuffer;
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
};
} // namespace vkr::resource
