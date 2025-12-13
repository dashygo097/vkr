#pragma once

#include "../buffers/command.hh"
#include "../ctx.hh"
#include "./universal.hh"

namespace vkr {

template <typename T> class UniformBufferBase {
public:
  UniformBufferBase(const T &object, VkDevice device,
                    VkPhysicalDevice physicalDevice)
      : object(object), device(device), physicalDevice(physicalDevice) {
    create();
  }

  UniformBufferBase(const T &object, const VulkanContext &ctx)
      : UniformBufferBase(object, ctx.device, ctx.physicalDevice) {}

  virtual ~UniformBufferBase() { destroy(); }

  UniformBufferBase(const UniformBufferBase &) = delete;
  UniformBufferBase &operator=(const UniformBufferBase &) = delete;

  void update(uint32_t currentFrame, const T &newObject) {
    if (currentFrame >= MAX_FRAMES_IN_FLIGHT) {
      throw std::out_of_range("currentFrame exceeds MAX_FRAMES_IN_FLIGHT");
    }
    if (_mapped[currentFrame] == nullptr) {
      throw std::runtime_error("Mapped memory is null for current frame");
    }
    memcpy(_mapped[currentFrame], &newObject, sizeof(T));
  }

  [[nodiscard]] const std::vector<VkBuffer> &buffers() const noexcept {
    return _uniformBuffers;
  }

  [[nodiscard]] const std::vector<VkDeviceMemory> &
  buffersMemory() const noexcept {
    return _memories;
  }

  [[nodiscard]] const std::vector<void *> &mapped() const noexcept {
    return _mapped;
  }

protected:
  void create() {
    VkDeviceSize bufferSize = sizeof(T);
    _uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    _memories.resize(MAX_FRAMES_IN_FLIGHT);
    _mapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   _uniformBuffers[i], _memories[i], device, physicalDevice);
      vkMapMemory(device, _memories[i], 0, bufferSize, 0, &_mapped[i]);
    }
  }

  void destroy() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      if (_mapped[i]) {
        vkUnmapMemory(device, _memories[i]);
        _mapped[i] = nullptr;
      }
      if (_memories[i] != VK_NULL_HANDLE) {
        vkFreeMemory(device, _memories[i], nullptr);
        _memories[i] = VK_NULL_HANDLE;
      }
      if (_uniformBuffers[i] != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, _uniformBuffers[i], nullptr);
        _uniformBuffers[i] = VK_NULL_HANDLE;
      }
    }
    _mapped.clear();
    _memories.clear();
    _uniformBuffers.clear();
  }

protected:
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  T object;

  std::vector<VkBuffer> _uniformBuffers{};
  std::vector<VkDeviceMemory> _memories{};
  std::vector<void *> _mapped{};
};

// default uniform buffer object
struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

class UniformBuffers : public UniformBufferBase<UniformBufferObject> {
public:
  using UniformBufferBase<UniformBufferObject>::UniformBufferBase;
};

// shadertoy uniform buffer object
struct ShadertoyUniformObject {
  glm::vec3 iResolution;
  float iTime;
  glm::vec4 iMouse;
  glm::vec4 iDate;
  float iTimeDelta;
  float iFrameRate;
  int iFrame;
};

class ShadertoyUniformBuffers
    : public UniformBufferBase<ShadertoyUniformObject> {
public:
  using UniformBufferBase<ShadertoyUniformObject>::UniformBufferBase;

  void updateTime(uint32_t currentFrame, float time) {
    auto obj = object;
    obj.iTime = time;
    update(currentFrame, obj);
  }

  void updateResolution(uint32_t currentFrame, int width, int height) {
    auto obj = object;
    obj.iResolution = glm::vec3(width, height, 1.0f);
    update(currentFrame, obj);
  }
};

} // namespace vkr
