#pragma once

#include "vkr/core/command/buffers.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffer/buffer.hh"
#include "vkr/resource/buffer/uniform_buffer.hh"
#include <array>
#include <cstddef>

namespace vkr::resource {

class IUniformBuffer {
public:
  virtual ~IUniformBuffer() = default;

  [[nodiscard]] virtual auto target(uint32_t frameIndex) const
      -> const Buffer & = 0;
  [[nodiscard]] virtual auto targetCount() const noexcept -> size_t = 0;
  [[nodiscard]] virtual auto mappedCount() const noexcept -> size_t = 0;
  [[nodiscard]] virtual auto bufferSize() const noexcept -> VkDeviceSize = 0;

  virtual void updateRaw(uint32_t frameIndex, const void *data,
                         size_t size) = 0;
};

template <typename UniformType>
class FrameUniformBuffers : public IUniformBuffer {
public:
  explicit FrameUniformBuffers(const core::Device &device)
      : targets_{UniformBuffer<UniformType>{device},
                 UniformBuffer<UniformType>{device}} {}

  ~FrameUniformBuffers() override = default;

  FrameUniformBuffers(const FrameUniformBuffers &) = delete;
  auto operator=(const FrameUniformBuffers &) -> FrameUniformBuffers & = delete;

  [[nodiscard]] auto target(uint32_t frameIndex) const
      -> const Buffer & override {
    if (frameIndex >= targets_.size()) {
      VKR_RES_ERROR("Uniform buffer frame {} is unavailable", frameIndex);
    }
    return targets_[frameIndex].target();
  }

  [[nodiscard]] auto targetCount() const noexcept -> size_t override {
    return targets_.size();
  }

  [[nodiscard]] auto mappedCount() const noexcept -> size_t override {
    size_t count{0};
    for (const auto &target : targets_) {
      if (target.isMapped()) {
        ++count;
      }
    }
    return count;
  }

  [[nodiscard]] auto bufferSize() const noexcept -> VkDeviceSize override {
    return sizeof(UniformType);
  }

  void updateRaw(uint32_t frameIndex, const void *data, size_t size) override {
    if (size != sizeof(UniformType)) {
      VKR_RES_ERROR("Size mismatch in frame uniform buffer update!");
    }
    update(frameIndex, *static_cast<const UniformType *>(data));
  }

  void update(uint32_t frameIndex, const UniformType &newObject) {
    if (frameIndex >= targets_.size()) {
      VKR_RES_ERROR("Uniform buffer frame {} is unavailable", frameIndex);
    }
    targets_[frameIndex].update(newObject);
  }

private:
  std::array<UniformBuffer<UniformType>, core::MAX_FRAMES_IN_FLIGHT> targets_;
};

} // namespace vkr::resource
