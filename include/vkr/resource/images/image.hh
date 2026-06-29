#pragma once

#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include <string>
#include <vulkan/vulkan.h>

namespace vkr::resource {

struct ImageDesc {
  std::string filePath{};

  uint32_t width{0};
  uint32_t height{0};
  uint32_t mipLevels{1};
  uint32_t arrayLayers{1};

  VkFormat format{VK_FORMAT_UNDEFINED};
  VkImageTiling tiling{VK_IMAGE_TILING_OPTIMAL};
  VkImageUsageFlags usage{0};
  VkMemoryPropertyFlags memoryProperties{VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
  VkImageAspectFlags aspectMask{VK_IMAGE_ASPECT_COLOR_BIT};
  VkImageLayout initialLayout{VK_IMAGE_LAYOUT_UNDEFINED};
  VkImageLayout finalLayout{VK_IMAGE_LAYOUT_UNDEFINED};
  VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};

  bool forceRgba{true};

  [[nodiscard]] auto hasFile() const noexcept -> bool {
    return !filePath.empty();
  }

  [[nodiscard]] static auto
  textureFile(const std::string &path,
              VkFormat format = VK_FORMAT_R8G8B8A8_SRGB) -> ImageDesc {
    ImageDesc desc{};
    desc.filePath = path;
    desc.format = format;
    desc.tiling = VK_IMAGE_TILING_OPTIMAL;
    desc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    desc.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    desc.forceRgba = true;
    return desc;
  }

  [[nodiscard]] static auto empty2D(
      uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
      VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT) -> ImageDesc {
    ImageDesc desc{};
    desc.width = width;
    desc.height = height;
    desc.format = format;
    desc.tiling = VK_IMAGE_TILING_OPTIMAL;
    desc.usage = usage;
    desc.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.aspectMask = aspectMask;
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    desc.finalLayout = finalLayout;
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    return desc;
  }

  [[nodiscard]] static auto sampled2D(uint32_t width, uint32_t height,
                                      VkFormat format) -> ImageDesc {
    return empty2D(width, height, format,
                   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                   VK_IMAGE_ASPECT_COLOR_BIT);
  }

  [[nodiscard]] static auto colorAttachment(uint32_t width, uint32_t height,
                                            VkFormat format) -> ImageDesc {
    return empty2D(
        width, height, format,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
  }

  [[nodiscard]] static auto depthAttachment(uint32_t width, uint32_t height,
                                            VkFormat format) -> ImageDesc {
    return empty2D(width, height, format,
                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                   VK_IMAGE_ASPECT_DEPTH_BIT);
  }
};

class Image {
public:
  explicit Image(const core::Device &device,
                 const core::CommandPool &commandPool);
  ~Image();

  Image(const Image &) = delete;
  auto operator=(const Image &) -> Image & = delete;

  void create();
  void destroy();
  void update(const ImageDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const ImageDesc & {
    return desc_;
  }
  [[nodiscard]] auto width() const noexcept -> uint32_t { return width_; }
  [[nodiscard]] auto height() const noexcept -> uint32_t { return height_; }
  [[nodiscard]] auto channels() const noexcept -> uint32_t { return channels_; }
  [[nodiscard]] auto layout() const noexcept -> VkImageLayout {
    return layout_;
  }
  [[nodiscard]] auto image() const noexcept -> VkImage { return vk_image_; }
  [[nodiscard]] auto memory() const noexcept -> VkDeviceMemory {
    return vk_memory_;
  }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  ImageDesc desc_{};

  uint32_t width_{0};
  uint32_t height_{0};
  uint32_t channels_{0};
  VkImageLayout layout_{VK_IMAGE_LAYOUT_UNDEFINED};

  VkImage vk_image_{VK_NULL_HANDLE};
  VkDeviceMemory vk_memory_{VK_NULL_HANDLE};

  // helpers
  void createImageObject(uint32_t width, uint32_t height, VkFormat format,
                         VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties, uint32_t mipLevels,
                         uint32_t arrayLayers, VkSampleCountFlagBits samples);
  void createFromFile();
  void createEmpty();

  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageAspectFlags aspectMask,
                             VkImageLayout oldLayout, VkImageLayout newLayout);
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height);

  [[nodiscard]] auto beginSingleTimeCommands() -> VkCommandBuffer;
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
};

} // namespace vkr::resource
