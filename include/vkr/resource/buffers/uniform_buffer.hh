#pragma once

#include "vkr/core/command/buffers.hh"
#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/buffer.hh"
#include "vkr/resource/buffers/ubos.hh"
#include <array>
#include <cstddef>
#include <cstring>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace vkr::resource {

class IUniformBuffer {
public:
  virtual ~IUniformBuffer() = default;

  [[nodiscard]] virtual auto target(uint32_t index) const -> const Buffer & = 0;
  [[nodiscard]] virtual auto targetCount() const noexcept -> size_t = 0;
  [[nodiscard]] virtual auto mappedCount() const noexcept -> size_t = 0;
  [[nodiscard]] virtual auto bufferSize() const noexcept -> VkDeviceSize = 0;

  virtual void updateRaw(uint32_t currentFrame, const void *data,
                         size_t size) = 0;
};

template <typename UniformType> class UniformBuffer : public IUniformBuffer {
public:
  explicit UniformBuffer(const core::Device &device)
      : device_(device), targets_{std::make_unique<Buffer>(device),
                                  std::make_unique<Buffer>(device)} {
    create();
  }

  ~UniformBuffer() override { destroy(); }

  UniformBuffer(const UniformBuffer &) = delete;
  auto operator=(const UniformBuffer &) -> UniformBuffer & = delete;

  [[nodiscard]] auto target(uint32_t index) const -> const Buffer & override {
    if (index >= targets_.size()) {
      VKR_RES_ERROR("Uniform buffer target {} is unavailable", index);
    }
    return *targets_[index];
  }

  [[nodiscard]] auto targetCount() const noexcept -> size_t override {
    return targets_.size();
  }

  [[nodiscard]] auto bufferSize() const noexcept -> VkDeviceSize override {
    return sizeof(UniformType);
  }

  [[nodiscard]] auto mappedCount() const noexcept -> size_t override {
    size_t count{0};
    for (const auto &target : targets_) {
      if (target->isMapped()) {
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
    if (!targets_[currentFrame]->isMapped()) {
      VKR_RES_ERROR("Mapped memory is null for current frame!");
    }
    memcpy(targets_[currentFrame]->mapped(), &newObject, sizeof(UniformType));
  }

protected:
  void create() {

    VkDeviceSize bufferSize = sizeof(UniformType);
    for (size_t i = 0; i < core::MAX_FRAMES_IN_FLIGHT; i++) {
      targets_[i]->update(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
      (void)targets_[i]->map(bufferSize);
    }
  }

  void destroy() {
    for (auto &target : targets_) {
      target->destroy();
    }
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  std::array<std::unique_ptr<Buffer>, core::MAX_FRAMES_IN_FLIGHT> targets_{};
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
