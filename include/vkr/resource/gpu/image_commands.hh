#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/buffers/buffer.hh"
#include "vkr/resource/gpu/image.hh"

namespace vkr::resource {

class ImageCommands {
public:
  static void transitionLayout(const core::Device &device,
                               const core::CommandPool &commandPool,
                               Image &image, VkImageLayout oldLayout,
                               VkImageLayout newLayout);

  static void copyBufferToImage(const core::Device &device,
                                const core::CommandPool &commandPool,
                                const Buffer &buffer, Image &image,
                                uint32_t layers, uint32_t channels);

private:
  [[nodiscard]] static auto
  beginSingleTimeCommands(const core::Device &device,
                          const core::CommandPool &commandPool)
      -> VkCommandBuffer;

  static void endSingleTimeCommands(const core::Device &device,
                                    const core::CommandPool &commandPool,
                                    VkCommandBuffer commandBuffer);
};

} // namespace vkr::resource
