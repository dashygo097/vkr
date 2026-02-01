#pragma once

#include "../../core/command_buffer.hh"
#include "../../core/device.hh"
#include "../../logger.hh"
#include "./buffer_utils.hh"
#include <glm/glm.hpp>

namespace vkr::resource {

class IUniformBuffer {
public:
  virtual ~IUniformBuffer() = default;

  virtual const std::vector<VkBuffer> &buffers() const noexcept = 0;
  virtual const std::vector<VkDeviceMemory> &buffersMemory() const noexcept = 0;
  virtual const std::vector<void *> &mapped() const noexcept = 0;

  virtual void updateRaw(uint32_t currentFrame, const void *data,
                         size_t size) = 0;
};

template <typename ObjectType> class UniformBufferBase : public IUniformBuffer {
public:
  explicit UniformBufferBase(const core::Device &device) : device_(device) {
    create();
  }

  virtual ~UniformBufferBase() { destroy(); }

  UniformBufferBase(const UniformBufferBase &) = delete;
  UniformBufferBase &operator=(const UniformBufferBase &) = delete;

  void update(uint32_t currentFrame, const ObjectType &newObject) {
    if (currentFrame >= MAX_FRAMES_IN_FLIGHT) {
      throw std::out_of_range("currentFrame exceeds MAX_FRAMES_IN_FLIGHT");
      VKR_RES_ERROR("currentFrame exceeds MAX_FRAMES_IN_FLIGHT({})!",
                    MAX_FRAMES_IN_FLIGHT);
    }
    if (mapped_[currentFrame] == nullptr) {
      throw std::runtime_error("Mapped memory is null for current frame");
    }
    memcpy(mapped_[currentFrame], &newObject, sizeof(ObjectType));
  }

  void updateRaw(uint32_t currentFrame, const void *data,
                 size_t size) override {
    if (size != sizeof(ObjectType)) {
      VKR_RES_ERROR("Size mismatch in uniform buffer update!");
    }
    update(currentFrame, *static_cast<const ObjectType *>(data));
  }

  [[nodiscard]] const std::vector<VkBuffer> &buffers() const noexcept override {
    return vk_uniform_buffers_;
  }

  [[nodiscard]] const std::vector<VkDeviceMemory> &
  buffersMemory() const noexcept override {
    return vk_memories_;
  }

  [[nodiscard]] const std::vector<void *> &mapped() const noexcept override {
    return mapped_;
  }

protected:
  void create() {
    VkDeviceSize bufferSize = sizeof(ObjectType);
    vk_uniform_buffers_.resize(MAX_FRAMES_IN_FLIGHT);
    vk_memories_.resize(MAX_FRAMES_IN_FLIGHT);
    mapped_.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   vk_uniform_buffers_[i], vk_memories_[i], device_.device(),
                   device_.physicalDevice());
      vkMapMemory(device_.device(), vk_memories_[i], 0, bufferSize, 0,
                  &mapped_[i]);
    }
  }

  void destroy() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      if (mapped_[i]) {
        vkUnmapMemory(device_.device(), vk_memories_[i]);
        mapped_[i] = nullptr;
      }
      if (vk_memories_[i] != VK_NULL_HANDLE) {
        vkFreeMemory(device_.device(), vk_memories_[i], nullptr);
        vk_memories_[i] = VK_NULL_HANDLE;
      }
      if (vk_uniform_buffers_[i] != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_.device(), vk_uniform_buffers_[i], nullptr);
        vk_uniform_buffers_[i] = VK_NULL_HANDLE;
      }
    }
    mapped_.clear();
    vk_memories_.clear();
    vk_uniform_buffers_.clear();
  }

protected:
  // dependencies
  const core::Device &device_;

  // components
  ObjectType _object;
  std::vector<VkBuffer> vk_uniform_buffers_{};
  std::vector<VkDeviceMemory> vk_memories_{};
  std::vector<void *> mapped_{};
};

// uniform buffer for 3d object
struct UniformBuffer3DObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

class UniformBuffer3D : public UniformBufferBase<UniformBuffer3DObject> {
public:
  using UniformBufferBase<UniformBuffer3DObject>::UniformBufferBase;
};

// shader toy uniform buffer
struct UniformBufferShaderToyObject {
  alignas(16) glm::vec3 iResolution;
  alignas(4) float iTime;
  alignas(4) float iTimeDelta;
  alignas(4) float iFrameRate;
  alignas(4) int iFrame;
  alignas(16) glm::vec4 iMouse;
  alignas(16) glm::vec4 iDate;
  alignas(16) glm::vec4 iChannelTime;
  alignas(16) glm::vec3 iChannelResolution[4];
};

class ShaderToyUniformBuffer
    : public UniformBufferBase<UniformBufferShaderToyObject> {
  using UniformBufferBase<UniformBufferShaderToyObject>::UniformBufferBase;
};

} // namespace vkr::resource
