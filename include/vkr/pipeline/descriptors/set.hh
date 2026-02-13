#pragma once
#include "../../core/device.hh"
#include "../../resources/buffers/uniform_buffer.hh"
#include "../../resources/textures/imageview.hh"
#include "../../resources/textures/sampler.hh"
#include "./binding.hh"
#include "./pool.hh"
#include "./writer.hh"
#include <memory>
#include <vector>

namespace vkr::pipeline {

class DescriptorSets {
public:
  DescriptorSets(const core::Device &device, DescriptorSetLayout &layout,
                 const DescriptorPool &pool,
                 uint32_t frameCount = core::MAX_FRAMES_IN_FLIGHT);
  ~DescriptorSets();

  DescriptorSets(const DescriptorSets &) = delete;
  DescriptorSets &operator=(const DescriptorSets &) = delete;

  void bindUniformBuffer(uint32_t binding, const std::vector<VkBuffer> &buffers,
                         VkDeviceSize size, VkDeviceSize offset = 0);

  void bindStorageBuffer(uint32_t binding, const std::vector<VkBuffer> &buffers,
                         VkDeviceSize size, VkDeviceSize offset = 0);

  void bindImageSampler(
      uint32_t binding, const std::vector<VkImageView> &imageViews,
      VkSampler sampler,
      VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  void bindToFrame(uint32_t frameIndex, DescriptorWriter &writer);

  [[nodiscard]] const std::vector<VkDescriptorSet> &sets() const noexcept {
    return sets_;
  }

  void bind(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t frameIndex,
            VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

private:
  // dependencies
  const core::Device &device_;
  DescriptorSetLayout &layout_;
  const DescriptorPool &pool_;
  uint32_t frame_count_{0};

  // components
  std::vector<VkDescriptorSet> sets_{};

  void allocateSets();
};

class DescriptorManager {
public:
  explicit DescriptorManager(const core::Device &device);
  ~DescriptorManager() = default;

  DescriptorManager(const DescriptorManager &) = delete;
  DescriptorManager &operator=(const DescriptorManager &) = delete;

  std::shared_ptr<DescriptorSetLayout>
  createLayout(const std::vector<DescriptorBinding> &bindings);

  std::unique_ptr<DescriptorSets>
  allocate(DescriptorSetLayout &layout, const DescriptorPool &pool,
           uint32_t frameCount = core::MAX_FRAMES_IN_FLIGHT);

  static DescriptorPoolSizes calculatePoolSizes(uint32_t maxSets);

private:
  // dependencies
  const core::Device &device_;
};

} // namespace vkr::pipeline
