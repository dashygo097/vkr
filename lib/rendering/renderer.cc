#include "vkr/rendering/renderer.hh"
#include <stdexcept>

namespace vkr::render {

Renderer::Renderer(core::Device &device, core::Swapchain &swapchain,
                   const core::CommandPool &commandPool,
                   const core::SyncObjects &syncObjects,
                   resource::ResourceManager &resourceManager,
                   const pipeline::RenderPass &renderPass)
    : _device(device), _swapchain(swapchain), _commandPool(commandPool),
      _syncObjects(syncObjects), _resourceManager(resourceManager),
      _renderPass(renderPass) {
  _commandBuffers = std::make_unique<core::CommandBuffers>(device, commandPool);
}

Renderer::~Renderer() = default;

bool Renderer::beginFrame(FrameData &outFrameData) {
  waitForFence(_currentFrame);

  uint32_t imageIndex;
  if (!acquireNextImage(imageIndex)) {
    return false;
  }

  resetFence(_currentFrame);

  vkResetCommandBuffer(_commandBuffers->commandBuffer(_currentFrame), 0);

  outFrameData.imageIndex = imageIndex;
  outFrameData.frameIndex = _currentFrame;
  outFrameData.commandBuffer = _commandBuffers->commandBuffer(_currentFrame);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(outFrameData.commandBuffer, &beginInfo) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  return true;
}

void Renderer::endFrame(const FrameData &frameData) {
  if (vkEndCommandBuffer(frameData.commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }

  submitCommandBuffer(frameData);
  present(frameData.imageIndex);
  _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::beginRenderPass(const FrameData &frameData) {
  auto framebuffers =
      _resourceManager.getFramebuffers("swapchain")->framebuffers();

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = _renderPass.renderPass();
  renderPassInfo.framebuffer = framebuffers[frameData.imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = _swapchain.extent2D();

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(frameData.commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void Renderer::endRenderPass(const FrameData &frameData) {
  vkCmdEndRenderPass(frameData.commandBuffer);
}

void Renderer::bindPipeline(
    const FrameData &frameData, VkPipeline pipeline,
    VkPipelineLayout pipelineLayout,
    const std::vector<VkDescriptorSet> &descriptorSets) {
  vkCmdBindPipeline(frameData.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline);

  if (!descriptorSets.empty()) {
    vkCmdBindDescriptorSets(frameData.commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
                            1, &descriptorSets[frameData.frameIndex], 0,
                            nullptr);
  }
}

void Renderer::setViewportAndScissor(const FrameData &frameData) {
  VkExtent2D extent = _swapchain.extent2D();

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(frameData.commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;
  vkCmdSetScissor(frameData.commandBuffer, 0, 1, &scissor);
}

void Renderer::drawGeometry(const FrameData &frameData) {
  auto vertexBuffers = _resourceManager.listVertexBuffers();
  auto indexBuffers = _resourceManager.listIndexBuffers();

  if (vertexBuffers.empty() || indexBuffers.empty()) {
    vkCmdDraw(frameData.commandBuffer, 3, 1, 0, 0);
    return;
  }

  for (size_t i = 0; i < vertexBuffers.size() && i < indexBuffers.size(); ++i) {
    if (vertexBuffers[i] && indexBuffers[i]) {
      VkBuffer vb = vertexBuffers[i]->buffer();
      VkDeviceSize offsets[] = {0};

      vkCmdBindVertexBuffers(frameData.commandBuffer, 0, 1, &vb, offsets);

      vkCmdBindIndexBuffer(frameData.commandBuffer, indexBuffers[i]->buffer(),
                           0, VK_INDEX_TYPE_UINT16);

      vkCmdDrawIndexed(frameData.commandBuffer,
                       static_cast<uint32_t>(indexBuffers[i]->indices().size()),
                       1, 0, 0, 0);
    }
  }
}

void Renderer::drawUI(const FrameData &frameData, ui::UI &ui) {
  ui.render(frameData.commandBuffer);
}

void Renderer::recreateSwapchain() {
  _device.waitIdle();

  _swapchain.recreate();
  _resourceManager.getFramebuffers("swapchain")->destroy();
  _resourceManager.getFramebuffers("swapchain")->create();
}

void Renderer::waitForFence(uint32_t frameIndex) {
  vkWaitForFences(_device.device(), 1,
                  &_syncObjects.inFlightFences()[frameIndex], VK_TRUE,
                  UINT64_MAX);
}

void Renderer::resetFence(uint32_t frameIndex) {
  vkResetFences(_device.device(), 1,
                &_syncObjects.inFlightFences()[frameIndex]);
}

bool Renderer::acquireNextImage(uint32_t &imageIndex) {
  VkResult result = vkAcquireNextImageKHR(
      _device.device(), _swapchain.swapchain(), UINT64_MAX,
      _syncObjects.imageAvailableSemaphores()[_currentFrame], VK_NULL_HANDLE,
      &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return false;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  return true;
}

void Renderer::submitCommandBuffer(const FrameData &frameData) {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {
      _syncObjects.imageAvailableSemaphores()[frameData.frameIndex]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &frameData.commandBuffer;

  VkSemaphore signalSemaphores[] = {
      _syncObjects.renderFinishedSemaphores()[frameData.imageIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(_device.graphicsQueue(), 1, &submitInfo,
                    _syncObjects.inFlightFences()[frameData.frameIndex]) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }
}

void Renderer::present(uint32_t imageIndex) {
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  VkSemaphore signalSemaphores[] = {
      _syncObjects.renderFinishedSemaphores()[imageIndex]};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapchains[] = {_swapchain.swapchain()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &imageIndex;

  VkResult result = vkQueuePresentKHR(_device.presentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      _framebufferResized) {
    _framebufferResized = false;
    recreateSwapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }
}

} // namespace vkr::render
