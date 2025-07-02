#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "ctx.hpp"
#include "impl/vertex.hpp"
#include "interface/command_buffers.hpp"
#include "interface/command_pool.hpp"
#include "interface/device.hpp"
#include "interface/frame_buffers.hpp"
#include "interface/instance.hpp"
#include "interface/pipeline.hpp"
#include "interface/render_pass.hpp"
#include "interface/surface.hpp"
#include "interface/swapchain.hpp"
#include "interface/sync_objects.hpp"
#include "interface/window.hpp"

class VulkanApplication {
public:
  VulkanApplication();
  virtual ~VulkanApplication();

  virtual void run() {
    initVulkan();
    mainLoop();
    cleanup();
  }

  VulkanContext ctx;
  std::unique_ptr<Window> window;
  std::unique_ptr<Instance> instance;
  std::unique_ptr<Surface> surface;
  std::unique_ptr<Device> device;

  std::unique_ptr<Swapchain> swapchain;
  std::unique_ptr<Framebuffers> swapchainFramebuffers;

  std::unique_ptr<RenderPass> renderPass;
  std::unique_ptr<GraphicsPipeline> graphicsPipeline;

  std::unique_ptr<CommandPool> commandPool;
  std::unique_ptr<VertexBuffer> vertexBuffer;
  std::unique_ptr<CommandBuffers> commandBuffers;

  std::unique_ptr<SyncObjects> syncObjects;

private:
  virtual void mainLoop();
  virtual void initVulkan();
  virtual void cleanup();
  virtual void drawFrame();
};
