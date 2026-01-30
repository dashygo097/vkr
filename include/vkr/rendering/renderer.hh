#pragma once

#include "../core/command_buffer.hh"
#include "../core/command_pool.hh"
#include "../core/device.hh"
#include "../core/swapchain.hh"
#include "../core/sync_objects.hh"
#include "../pipeline/render_pass.hh"
#include "../resources/manager.hh"
#include "../ui/ui.hh"

namespace vkr::render {

struct FrameData {
  uint32_t imageIndex;
  uint32_t frameIndex;
  VkCommandBuffer commandBuffer;
};

class Renderer {
public:
  explicit Renderer(core::Device &device, core::Swapchain &swapchain,
                    const core::CommandPool &commandPool,
                    const core::SyncObjects &syncObjects,
                    resource::ResourceManager &resourceManager,
                    const pipeline::RenderPass &renderPass);

  ~Renderer();

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  bool beginFrame(FrameData &outFrameData);
  void endFrame(const FrameData &frameData);

  void beginRenderPass(const FrameData &frameData);
  void endRenderPass(const FrameData &frameData);

  void bindPipeline(const FrameData &frameData, VkPipeline pipeline,
                    VkPipelineLayout pipelineLayout,
                    const std::vector<VkDescriptorSet> &descriptorSets);

  void drawGeometry(const FrameData &frameData);
  void drawUI(const FrameData &frameData, ui::UI &ui);

  void setViewportAndScissor(const FrameData &frameData);
  void recreateSwapchain();

  [[nodiscard]] uint32_t currentFrameIndex() const noexcept {
    return _currentFrame;
  }

private:
  // dependencies
  core::Device &_device;
  core::Swapchain &_swapchain;
  const core::CommandPool &_commandPool;
  const core::SyncObjects &_syncObjects;
  resource::ResourceManager &_resourceManager;
  const pipeline::RenderPass &_renderPass;

  // components
  std::unique_ptr<core::CommandBuffers> _commandBuffers;
  VkViewport _viewport{};
  VkRect2D _scissor{};

  // state
  uint32_t _currentFrame = 0;
  bool _framebufferResized = false;

  // methods
  void waitForFence(uint32_t frameIndex);
  void resetFence(uint32_t frameIndex);
  bool acquireNextImage(uint32_t &imageIndex);
  void submitCommandBuffer(const FrameData &frameData);
  void present(uint32_t imageIndex);
};

} // namespace vkr::render
