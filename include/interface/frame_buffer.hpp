#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class FrameBuffer {
public:
  FrameBuffer();
  ~FrameBuffer();

private:
  std::vector<VkFramebuffer> frameBuffers;
  bool frameBufferResized = false;
};
