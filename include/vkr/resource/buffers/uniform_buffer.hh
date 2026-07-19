#pragma once

#include "vkr/core/command/buffers.hh"
#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/buffer.hh"
#include "vkr/resource/buffers/ubos.hh"
#include <cstddef>
#include <cstring>
#include <glm/glm.hpp>
#include <vector>

namespace vkr::resource {

class IUniformBuffer {
public:
  virtual ~IUniformBuffer() = default;

  [[nodiscard]] virtual auto targets() const noexcept
      -> const std::vector<Buffer> & = 0;
  [[nodiscard]] virtual auto mappedCount() const noexcept -> size_t = 0;
  [[nodiscard]] virtual auto bufferSize() const noexcept -> VkDeviceSize = 0;

  virtual void updateRaw(uint32_t currentFrame, const void *data,
                         size_t size) = 0;
};

template <typename UniformType> class UniformBuffer : public IUniformBuffer {
public:
  explicit UniformBuffer(const core::Device &device) : device_(device) {
    create();
  }

  ~UniformBuffer() override { destroy(); }

  UniformBuffer(const UniformBuffer &) = delete;
  auto operator=(const UniformBuffer &) -> UniformBuffer & = delete;

  [[nodiscard]] auto targets() const noexcept
      -> const std::vector<Buffer> & override {
    return targets_;
  }

  [[nodiscard]] auto bufferSize() const noexcept -> VkDeviceSize override {
    return sizeof(UniformType);
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

  void updateRaw(uint32_t currentFrame, const void *data,
                 size_t size) override {
    if (size != sizeof(UniformType)) {
      VKR_RES_ERROR("Size mismatch in uniform buffer update!");
    }
    update(currentFrame, *static_cast<const UniformType *>(data));
  }

  void update(uint32_t currentFrame, const UniformType &newObject) {
    if (currentFrame >= core::MAX_FRAMES_IN_FLIGHT) {
      VKR_RES_ERROR("currentFrame exceeds MAX_FRAMES_IN_FLIGHT({})!",
                    core::MAX_FRAMES_IN_FLIGHT);
    }
    if (currentFrame >= targets_.size()) {
      VKR_RES_ERROR("Uniform buffer frame {} is unavailable", currentFrame);
    }
    if (!targets_[currentFrame].isMapped()) {
      VKR_RES_ERROR("Mapped memory is null for current frame!");
    }
    memcpy(targets_[currentFrame].mapped(), &newObject, sizeof(UniformType));
  }

protected:
  void create() {

    VkDeviceSize bufferSize = sizeof(UniformType);
    targets_.reserve(core::MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < core::MAX_FRAMES_IN_FLIGHT; i++) {
      targets_.emplace_back(device_);
      auto &target = targets_.back();
      target.create(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
      (void)target.map(bufferSize);
    }
  }

  void destroy() {
    for (auto &target : targets_) {
      target.unmap();
    }
    targets_.clear();
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  std::vector<Buffer> targets_{};
};

class UniformBuffer3D : public UniformBuffer<UniformBuffer3DObject> {
public:
  using UniformBuffer<UniformBuffer3DObject>::UniformBuffer;
};

class ShaderToyUniformBuffer
    : public UniformBuffer<UniformBufferShaderToyObject> {
  using UniformBuffer<UniformBufferShaderToyObject>::UniformBuffer;
};

} // namespace vkr::resource
