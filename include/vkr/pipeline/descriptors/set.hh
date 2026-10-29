#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include <cstdint>
#include <vector>

namespace vkr::res {
class Buffer;
class BufferSlice;
class Texture;
class Cubemap;
} // namespace vkr::res

namespace vkr::pipe {

class DescriptorPool;
class ColorAttachment;
class DepthAttachment;

class DescriptorSet {
public:
  explicit DescriptorSet(const core::Device &device, const DescriptorPool &pool,
                         const DescriptorSetLayout &layout);
  ~DescriptorSet();

  DescriptorSet(const DescriptorSet &) = delete;
  auto operator=(const DescriptorSet &) -> DescriptorSet & = delete;

  DescriptorSet(DescriptorSet &&other) noexcept;
  auto operator=(DescriptorSet &&) -> DescriptorSet & = delete;

  void create();
  void destroy() noexcept;
  void update();

  auto uniform(uint32_t binding, const res::Buffer &buffer) -> DescriptorSet &;
  auto uniform(uint32_t binding, const res::BufferSlice &slice)
      -> DescriptorSet &;
  auto storage(uint32_t binding, const res::Buffer &buffer) -> DescriptorSet &;
  auto storage(uint32_t binding, const res::BufferSlice &slice)
      -> DescriptorSet &;
  auto texture(uint32_t binding, const res::Texture &texture)
      -> DescriptorSet &;
  auto texture(uint32_t binding, const res::Cubemap &cubemap)
      -> DescriptorSet &;
  auto texture(uint32_t binding, const ColorAttachment &attachment)
      -> DescriptorSet &;
  auto texture(uint32_t binding, const DepthAttachment &attachment)
      -> DescriptorSet &;

  [[nodiscard]] auto set() const noexcept -> VkDescriptorSet { return set_; }
  [[nodiscard]] auto valid() const noexcept -> bool {
    return set_ != VK_NULL_HANDLE;
  }

private:
  friend class DescriptorPool;

  template <typename Info> struct Binding {
    uint32_t binding;
    VkDescriptorType type;
    Info info{};
    bool dirty{true};
  };

  // dependencies
  const core::Device &device_;
  const DescriptorPool &pool_;
  const DescriptorSetLayout &layout_;

  // components
  VkDescriptorSet set_{VK_NULL_HANDLE};
  std::vector<Binding<VkDescriptorBufferInfo>> buffers_{};
  std::vector<Binding<VkDescriptorImageInfo>> images_{};
  std::vector<VkWriteDescriptorSet> writes_{};

  void assign(uint32_t binding, VkDescriptorType type,
              VkDescriptorBufferInfo info);
  void assign(uint32_t binding, VkDescriptorType type,
              VkDescriptorImageInfo info);
  void validateBinding(uint32_t binding, VkDescriptorType type) const;
  void validateBuffer(VkDescriptorType type,
                      const VkDescriptorBufferInfo &info) const;
  void validateImage(const VkDescriptorImageInfo &info);
};

} // namespace vkr::pipe
