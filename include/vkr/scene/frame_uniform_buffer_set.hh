#pragma once

#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffer/uniform_buffer.hh"
#include <cstddef>
#include <memory>
#include <vector>

namespace vkr::scene {

class IFrameUniformBufferSet {
public:
  virtual ~IFrameUniformBufferSet() = default;

  [[nodiscard]] virtual auto descriptorInfo(uint32_t frameIndex) const
      -> VkDescriptorBufferInfo = 0;
  [[nodiscard]] virtual auto frameCount() const noexcept -> size_t = 0;
  [[nodiscard]] virtual auto mappedFrameCount() const noexcept -> size_t = 0;
  [[nodiscard]] virtual auto byteSize() const noexcept -> VkDeviceSize = 0;

  virtual void updateRaw(uint32_t frameIndex, const void *data,
                         size_t size) = 0;
};

template <typename UniformType>
class FrameUniformBufferSet final : public IFrameUniformBufferSet {
public:
  explicit FrameUniformBufferSet(const core::Device &device,
                                 uint32_t frameCount)
      : device_(device) {
    if (frameCount == 0) {
      VKR_RES_ERROR("FrameUniformBufferSet frame count must be greater than "
                    "zero");
    }

    buffers_.reserve(frameCount);
    for (uint32_t i = 0; i < frameCount; ++i) {
      buffers_.push_back(std::make_unique<resource::UniformBuffer<UniformType>>(
          device_));
    }
  }

  ~FrameUniformBufferSet() override = default;

  FrameUniformBufferSet(const FrameUniformBufferSet &) = delete;
  auto operator=(const FrameUniformBufferSet &)
      -> FrameUniformBufferSet & = delete;

  [[nodiscard]] auto descriptorInfo(uint32_t frameIndex) const
      -> VkDescriptorBufferInfo override {
    if (frameIndex >= buffers_.size()) {
      VKR_RES_ERROR("Uniform buffer frame {} is unavailable", frameIndex);
    }
    return buffers_[frameIndex]->descriptorInfo();
  }

  [[nodiscard]] auto frameCount() const noexcept -> size_t override {
    return buffers_.size();
  }

  [[nodiscard]] auto mappedFrameCount() const noexcept -> size_t override {
    size_t count{0};
    for (const auto &buffer : buffers_) {
      if (buffer->isMapped()) {
        ++count;
      }
    }
    return count;
  }

  [[nodiscard]] auto byteSize() const noexcept -> VkDeviceSize override {
    return sizeof(UniformType);
  }

  void updateRaw(uint32_t frameIndex, const void *data, size_t size) override {
    if (size != sizeof(UniformType)) {
      VKR_RES_ERROR("Size mismatch in frame uniform buffer update!");
    }
    update(frameIndex, *static_cast<const UniformType *>(data));
  }

  void update(uint32_t frameIndex, const UniformType &object) {
    if (frameIndex >= buffers_.size()) {
      VKR_RES_ERROR("Uniform buffer frame {} is unavailable", frameIndex);
    }
    buffers_[frameIndex]->update(object);
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  std::vector<std::unique_ptr<resource::UniformBuffer<UniformType>>> buffers_;
};

} // namespace vkr::scene
