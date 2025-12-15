#pragma once
#include "../../ctx.hh"
#include "../../resources/buffers/uniform_buffer.hh"
#include "./descriptor_binding.hh"
#include "./descriptor_pool.hh"
#include "./descriptor_writer.hh"
#include <memory>
#include <vector>

namespace vkr {

class Descriptor {
public:
  Descriptor(VkDevice device, const DescriptorSetLayout &layout,
             DescriptorPool &pool, uint32_t frameCount = MAX_FRAMES_IN_FLIGHT);

  Descriptor(const VulkanContext &ctx, const DescriptorSetLayout &layout,
             DescriptorPool &pool);

  ~Descriptor();

  Descriptor(const Descriptor &) = delete;
  Descriptor &operator=(const Descriptor &) = delete;

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
  [[nodiscard]] const DescriptorPool &pool() const noexcept { return *_pool; }

  void bind(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t frameIndex,
            VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

private:
  // dependencies
  VkDevice device{VK_NULL_HANDLE};

  // components
  DescriptorPool *_pool{nullptr};
  std::vector<VkDescriptorSet> _sets;
  uint32_t _frameCount{0};

  void allocateSets(VkDescriptorSetLayout layout);
};

class DescriptorManager {
public:
  DescriptorManager(VkDevice device,
                    uint32_t maxSets = MAX_FRAMES_IN_FLIGHT * 4);
  DescriptorManager(const VulkanContext &ctx,
                    uint32_t maxSets = MAX_FRAMES_IN_FLIGHT * 4);
  ~DescriptorManager() = default;

  DescriptorManager(const DescriptorManager &) = delete;
  DescriptorManager &operator=(const DescriptorManager &) = delete;

  std::shared_ptr<DescriptorSetLayout>
  createLayout(const std::vector<DescriptorBinding> &bindings);

  std::unique_ptr<Descriptor>
  allocate(const DescriptorSetLayout &layout,
           uint32_t frameCount = MAX_FRAMES_IN_FLIGHT);

  void reset();

  [[nodiscard]] DescriptorPool &pool() noexcept { return _pool; }

private:
  VkDevice device{VK_NULL_HANDLE};
  DescriptorPool _pool;

  DescriptorPoolSizes calculatePoolSizes(uint32_t maxSets);
};

} // namespace vkr
