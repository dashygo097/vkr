#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/descriptors/binding.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/writer.hh"
#include "vkr/resources/buffers/uniform_buffer.hh"
#include "vkr/resources/images/imageview.hh"
#include "vkr/resources/images/sampler.hh"
#include "vkr/resources/manager.hh"
#include <memory>
#include <vector>

namespace vkr::pipeline {

class DescriptorSets {
public:
  DescriptorSets(const core::Device &device,
                 resource::ResourceManager &resourceManager,
                 DescriptorSetLayout &layout, const DescriptorPool &pool,
                 uint32_t frameCount = core::MAX_FRAMES_IN_FLIGHT);
  ~DescriptorSets();

  DescriptorSets(const DescriptorSets &) = delete;
  auto operator=(const DescriptorSets &) -> DescriptorSets & = delete;

  void autoBindResources();

  void bindUniformBuffer(uint32_t binding, const std::vector<VkBuffer> &buffers,
                         VkDeviceSize size, VkDeviceSize offset = 0);

  void bindStorageBuffer(uint32_t binding, const std::vector<VkBuffer> &buffers,
                         VkDeviceSize size, VkDeviceSize offset = 0);

  void bindImageSampler(
      uint32_t binding, const std::vector<VkImageView> &imageViews,
      VkSampler sampler,
      VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  void bindToFrame(uint32_t frameIndex, DescriptorWriter &writer);

  [[nodiscard]] auto sets() const noexcept
      -> const std::vector<VkDescriptorSet> & {
    return sets_;
  }

  void bind(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t frameIndex,
            VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

private:
  // dependencies
  const core::Device &device_;
  resource::ResourceManager &resource_manager_;
  DescriptorSetLayout &layout_;
  const DescriptorPool &pool_;
  uint32_t frame_count_{0};

  // components
  std::vector<VkDescriptorSet> sets_{};

  void allocateSets();
};

} // namespace vkr::pipeline
