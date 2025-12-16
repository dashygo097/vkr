#pragma once
#include "../../ctx.hh"
#include "../../resources/buffers/uniform_buffer.hh"
#include "./descriptor_binding.hh"
#include "./descriptor_pool.hh"
#include "./descriptor_writer.hh"
#include <memory>
#include <vector>

namespace vkr {

class DescriptorSets {
public:
  DescriptorSets(VkDevice device, VkDescriptorSetLayout layout,
                 VkDescriptorPool pool,
                 uint32_t frameCount = MAX_FRAMES_IN_FLIGHT);

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
    return _sets;
  }

  void bind(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t frameIndex,
            VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};
  VkDescriptorSetLayout layout{VK_NULL_HANDLE};
  VkDescriptorPool pool{VK_NULL_HANDLE};

  // components
  std::vector<VkDescriptorSet> _sets;
  uint32_t _frameCount{0};

  void allocateSets();
};

class DescriptorManager {
public:
  explicit DescriptorManager(VkDevice device);
  explicit DescriptorManager(const VulkanContext &ctx);
  ~DescriptorManager() = default;

  DescriptorManager(const DescriptorManager &) = delete;
  DescriptorManager &operator=(const DescriptorManager &) = delete;

  std::shared_ptr<DescriptorSetLayout>
  createLayout(const std::vector<DescriptorBinding> &bindings);

  std::unique_ptr<DescriptorSets>
  allocate(VkDescriptorSetLayout layout, VkDescriptorPool pool,
           uint32_t frameCount = MAX_FRAMES_IN_FLIGHT);

  static DescriptorPoolSizes calculatePoolSizes(uint32_t maxSets);

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};
};

} // namespace vkr
