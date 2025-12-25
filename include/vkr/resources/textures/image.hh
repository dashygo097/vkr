#pragma once

#include "../../core/command_pool.hh"
#include "../../core/device.hh"
#include <stb_image.h>

namespace vkr {
class Image {
public:
  explicit Image(const Device &device, const CommandPool &commandPool);
  ~Image();

  Image(const Image &) = delete;
  Image &operator=(const Image &) = delete;

  void create(const std::string &imageFilePath);
  void destroy();
  void update(const std::string &imageFilePath);

  [[nodiscard]] int width() const { return _width; }
  [[nodiscard]] int height() const { return _height; }
  [[nodiscard]] int channels() const { return _channels; }
  [[nodiscard]] VkImage image() const { return _image; }
  [[nodiscard]] VkDeviceMemory imageMemory() const { return _imageMemory; }

private:
  // dependencies
  const Device &device;
  const CommandPool &commandPool;

  // components
  int _width;
  int _height;
  int _channels;
  VkImage _image;
  VkDeviceMemory _imageMemory;

  void createImage(uint32_t width, uint32_t height, VkFormat format,
                   VkImageTiling tiling, VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkImage &image,
                   VkDeviceMemory &imageMemory);

  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout);

  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height);

  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
};
} // namespace vkr
