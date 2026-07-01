#include "vkr/render/renderer.hh"
#include "vkr/logger.hh"

namespace vkr::render {

Renderer::Renderer(core::Device &device, core::Swapchain &swapchain,
                   const core::CommandPool &commandPool,
                   core::SyncObjects &syncObjects,
                   resource::ResourceManager &resourceManager, ui::UI &ui)
    : device_(device), swapchain_(swapchain), command_pool_(commandPool),
      sync_objects_(syncObjects), resource_manager_(resourceManager), ui_(ui) {
  command_buffers_ =
      std::make_unique<core::CommandBuffers>(device_, command_pool_);
}

auto Renderer::beginFrame(FrameData &outFrameData) -> bool {
  waitForFence(current_frame_);

  uint32_t imageIndex = 0;
  if (!acquireNextImage(imageIndex)) {
    return false;
  }

  resetFence(current_frame_);

  VkCommandBuffer commandBuffer = command_buffers_->buffer(current_frame_);

  vkResetCommandBuffer(commandBuffer, 0);

  outFrameData.imageIndex = imageIndex;
  outFrameData.frameIndex = current_frame_;
  outFrameData.commandBuffer = commandBuffer;

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(outFrameData.commandBuffer, &beginInfo) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer");
  }

  return true;
}

void Renderer::endFrame(const FrameData &frameData) {
  if (vkEndCommandBuffer(frameData.commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer");
  }

  submitCommandBuffer(frameData);
  present(frameData.imageIndex);

  current_frame_ = (current_frame_ + 1) % core::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::beginPass(const FrameData &frameData,
                         const resource::FramebufferSet &framebufferSet,
                         const pipeline::RenderPass &renderPass,
                         const RenderPassBeginDesc &desc) {
  if (desc.framebufferIndex >= framebufferSet.buffers().size()) {
    VKR_RENDER_ERROR("Framebuffer index {} out of range, framebuffer count {}",
                     desc.framebufferIndex, framebufferSet.buffers().size());
  }

  if (desc.renderArea.extent.width == 0 || desc.renderArea.extent.height == 0) {
    VKR_RENDER_ERROR("RenderPassBeginDesc has invalid extent: {}x{}",
                     desc.renderArea.extent.width,
                     desc.renderArea.extent.height);
  }

  VkRenderPassBeginInfo info{};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  info.renderPass = renderPass.renderPass();
  info.framebuffer = framebufferSet.buffer(desc.framebufferIndex);
  info.renderArea = desc.renderArea;
  info.clearValueCount = static_cast<uint32_t>(desc.clearValues.size());
  info.pClearValues =
      desc.clearValues.empty() ? nullptr : desc.clearValues.data();

  vkCmdBeginRenderPass(frameData.commandBuffer, &info, desc.contents);
}

void Renderer::endPass(const FrameData &frameData) {
  vkCmdEndRenderPass(frameData.commandBuffer);
}

void Renderer::beginSwapchainPass(
    const FrameData &frameData, const resource::FramebufferSet &framebufferSet,
    const pipeline::RenderPass &renderPass) {

  std::vector<VkClearValue> clearValues(2);
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  RenderPassBeginDesc desc{};
  desc.framebufferIndex = frameData.imageIndex;
  desc.renderArea = {
      .offset = {0, 0},
      .extent = swapchain_.extent2D(),
  };
  desc.clearValues = std::move(clearValues);

  beginPass(frameData, framebufferSet, renderPass, desc);
}

void Renderer::beginOffscreenPass(
    const FrameData &frameData, const resource::FramebufferSet &framebufferSet,
    const pipeline::RenderPass &renderPass,
    const resource::OffscreenTarget &target) {
  std::vector<VkClearValue> clearValues(2);
  clearValues[0].color = {{0.05f, 0.05f, 0.05f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  RenderPassBeginDesc desc{};
  desc.framebufferIndex = 0;
  desc.renderArea = {
      .offset = {0, 0},
      .extent = target.extent2D(),
  };
  desc.clearValues = std::move(clearValues);

  beginPass(frameData, framebufferSet, renderPass, desc);
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

void Renderer::setViewportAndScissor(const FrameData &frameData,
                                     VkExtent2D extent) {
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

void Renderer::setViewportAndScissor(const FrameData &frameData) {
  setViewportAndScissor(frameData, swapchain_.extent2D());
}

void Renderer::setOffscreenViewportAndScissor(
    const FrameData &frameData, const resource::OffscreenTarget &target) {
  setViewportAndScissor(frameData, target.extent2D());
}

void Renderer::drawGeometry(const FrameData &frameData) {
  auto vertexBuffers = resource_manager_.listVertexBuffers();
  auto indexBuffers = resource_manager_.listIndexBuffers();

  if (vertexBuffers.empty() || indexBuffers.empty()) {
    vkCmdDraw(frameData.commandBuffer, 3, 1, 0, 0);
    return;
  }

  for (size_t i = 0; i < vertexBuffers.size() && i < indexBuffers.size(); i++) {
    if (!vertexBuffers[i] || !indexBuffers[i]) {
      continue;
    }

    VkBuffer vertexBuffer = vertexBuffers[i]->buffer();
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(frameData.commandBuffer, 0, 1, &vertexBuffer,
                           offsets);

    vkCmdBindIndexBuffer(frameData.commandBuffer, indexBuffers[i]->buffer(), 0,
                         VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(frameData.commandBuffer,
                     static_cast<uint32_t>(indexBuffers[i]->indices().size()),
                     1, 0, 0, 0);
  }
}

void Renderer::drawUI(const FrameData &frameData) {
  ui_.render(frameData.commandBuffer);
}

void Renderer::waitForFence(uint32_t frameIndex) {
  vkWaitForFences(device_.device(), 1,
                  &sync_objects_.inFlightFences()[frameIndex], VK_TRUE,
                  UINT64_MAX);
}

void Renderer::resetFence(uint32_t frameIndex) {
  vkResetFences(device_.device(), 1,
                &sync_objects_.inFlightFences()[frameIndex]);
}

auto Renderer::acquireNextImage(uint32_t &imageIndex) -> bool {
  VkResult result = vkAcquireNextImageKHR(
      device_.device(), swapchain_.swapchain(), UINT64_MAX,
      sync_objects_.imageAvailableSemaphores()[current_frame_], VK_NULL_HANDLE,
      &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return false;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image");
  }

  return true;
}

void Renderer::submitCommandBuffer(const FrameData &frameData) {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {
      sync_objects_.imageAvailableSemaphores()[frameData.frameIndex]};

  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &frameData.commandBuffer;

  VkSemaphore signalSemaphores[] = {
      sync_objects_.renderFinishedSemaphores()[frameData.imageIndex]};

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(device_.graphicsQueue(), 1, &submitInfo,
                    sync_objects_.inFlightFences()[frameData.frameIndex]) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer");
  }
}

void Renderer::present(uint32_t imageIndex) {
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  VkSemaphore signalSemaphores[] = {
      sync_objects_.renderFinishedSemaphores()[imageIndex]};

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapchains[] = {swapchain_.swapchain()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &imageIndex;

  VkResult result = vkQueuePresentKHR(device_.presentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return;
  }

  if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image");
  }
}

} // namespace vkr::render
