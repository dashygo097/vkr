#include "vkr/render/renderer.hh"
#include "vkr/logger.hh"

namespace vkr::render {

Renderer::Renderer(const core::Device &device, const core::Swapchain &swapchain,
                   const core::CommandPool &commandPool,
                   core::SyncObjects &syncObjects,
                   resource::ResourceManager &resourceManager, ui::UI &ui)
    : device_(device), swapchain_(swapchain), command_pool_(commandPool),
      sync_objects_(syncObjects), resource_manager_(resourceManager), ui_(ui) {
  command_buffers_ =
      std::make_unique<core::CommandBuffers>(device_, command_pool_);
}

auto Renderer::beginFrame() -> bool {
  ensureFrameInactive("beginFrame");

  sync_objects_.waitForFrame(current_frame_);

  uint32_t imageIndex = 0;
  if (!acquireNextImage(imageIndex)) {
    return false;
  }

  sync_objects_.resetFrame(current_frame_);

  VkCommandBuffer commandBuffer = command_buffers_->buffer(current_frame_);
  vkResetCommandBuffer(commandBuffer, 0);

  image_index_ = imageIndex;
  frame_index_ = current_frame_;
  command_buffer_ = commandBuffer;

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(command_buffer_, &beginInfo) != VK_SUCCESS) {
    VKR_RENDER_ERROR("failed to begin recording command buffer");
  }

  frame_active_ = true;
  return true;
}

void Renderer::endFrame() {
  ensureFrameActive("endFrame");

  if (vkEndCommandBuffer(command_buffer_) != VK_SUCCESS) {
    VKR_RENDER_ERROR("failed to record command buffer");
  }

  submitCommandBuffer();
  present(image_index_);

  current_frame_ = (current_frame_ + 1) % core::MAX_FRAMES_IN_FLIGHT;

  image_index_ = 0;
  frame_index_ = 0;
  command_buffer_ = VK_NULL_HANDLE;
  frame_active_ = false;
}

void Renderer::beginPass(const resource::FramebufferSet &framebufferSet,
                         const pipeline::RenderPass &renderPass,
                         const RenderPassBeginDesc &desc) {
  ensureFrameActive("beginPass");

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

  vkCmdBeginRenderPass(command_buffer_, &info, desc.contents);
}

void Renderer::endPass() {
  ensureFrameActive("endPass");
  vkCmdEndRenderPass(command_buffer_);
}

void Renderer::beginSwapchainPass(
    const resource::FramebufferSet &framebufferSet,
    const pipeline::RenderPass &renderPass,
    const resource::SwapchainTarget &swapchain) {
  ensureFrameActive("beginSwapchainPass");

  std::vector<VkClearValue> clearValues(2);
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  RenderPassBeginDesc desc{};
  desc.framebufferIndex = image_index_;
  desc.renderArea = {
      .offset = {0, 0},
      .extent = {swapchain.width(), swapchain.height()},
  };
  desc.clearValues = std::move(clearValues);

  beginPass(framebufferSet, renderPass, desc);
}

void Renderer::beginOffscreenPass(
    const resource::FramebufferSet &framebufferSet,
    const pipeline::RenderPass &renderPass,
    const resource::OffscreenTarget &target) {
  ensureFrameActive("beginOffscreenPass");

  std::vector<VkClearValue> clearValues(2);
  clearValues[0].color = {{0.05f, 0.05f, 0.05f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  RenderPassBeginDesc desc{};
  desc.framebufferIndex = 0;
  desc.renderArea = {
      .offset = {0, 0},
      .extent = {target.width(), target.height()},
  };
  desc.clearValues = std::move(clearValues);

  beginPass(framebufferSet, renderPass, desc);
}

void Renderer::bindPipeline(
    VkPipeline pipeline, VkPipelineLayout pipelineLayout,
    const std::vector<VkDescriptorSet> &descriptorSets) {
  ensureFrameActive("bindPipeline");

  if (pipeline == VK_NULL_HANDLE) {
    VKR_RENDER_ERROR("bindPipeline received null VkPipeline");
  }

  if (pipelineLayout == VK_NULL_HANDLE) {
    VKR_RENDER_ERROR("bindPipeline received null VkPipelineLayout");
  }

  vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  if (descriptorSets.empty()) {
    return;
  }

  if (frame_index_ >= descriptorSets.size()) {
    VKR_RENDER_ERROR("Descriptor set frame index {} out of range, count {}",
                     frame_index_, descriptorSets.size());
  }

  VkDescriptorSet descriptorSet = descriptorSets[frame_index_];

  vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
}

void Renderer::setViewportAndScissor(VkExtent2D extent) {
  ensureFrameActive("setViewportAndScissor");

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(command_buffer_, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;

  vkCmdSetScissor(command_buffer_, 0, 1, &scissor);
}

void Renderer::setViewportAndScissor() {
  setViewportAndScissor({swapchain_.width(), swapchain_.height()});
}

void Renderer::setOffscreenViewportAndScissor(
    const resource::OffscreenTarget &target) {
  setViewportAndScissor({target.width(), target.height()});
}

void Renderer::drawGeometry() {
  ensureFrameActive("drawGeometry");

  auto vertexBuffers = resource_manager_.listVertexBuffers();
  auto indexBuffers = resource_manager_.listIndexBuffers();

  if (vertexBuffers.empty() || indexBuffers.empty()) {
    vkCmdDraw(command_buffer_, 3, 1, 0, 0);
    return;
  }

  for (size_t i = 0; i < vertexBuffers.size() && i < indexBuffers.size(); i++) {
    if (!vertexBuffers[i] || !indexBuffers[i]) {
      continue;
    }

    VkBuffer vertexBuffer = vertexBuffers[i]->buffer();
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(command_buffer_, 0, 1, &vertexBuffer, offsets);

    vkCmdBindIndexBuffer(command_buffer_, indexBuffers[i]->buffer(), 0,
                         VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(command_buffer_,
                     static_cast<uint32_t>(indexBuffers[i]->indices().size()),
                     1, 0, 0, 0);
  }
}

void Renderer::drawUI() {
  ensureFrameActive("drawUI");
  ui_.render(command_buffer_);
}

void Renderer::ensureFrameActive(const char *op) const {
  if (!frame_active_) {
    VKR_RENDER_ERROR("Renderer::{} called without an active frame", op);
  }
}

void Renderer::ensureFrameInactive(const char *op) const {
  if (frame_active_) {
    VKR_RENDER_ERROR("Renderer::{} called while a frame is already active", op);
  }
}

auto Renderer::acquireNextImage(uint32_t &imageIndex) -> bool {
  VkResult result = vkAcquireNextImageKHR(
      device_.device(), swapchain_.swapchain(), UINT64_MAX,
      sync_objects_.imageAvailableSemaphore(current_frame_), VK_NULL_HANDLE,
      &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return false;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    VKR_RENDER_ERROR("failed to acquire swap chain image");
  }

  return true;
}

void Renderer::submitCommandBuffer() {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {
      sync_objects_.imageAvailableSemaphore(frame_index_)};

  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &command_buffer_;

  VkSemaphore signalSemaphores[] = {
      sync_objects_.renderFinishedSemaphore(image_index_)};

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(device_.graphicsQueue(), 1, &submitInfo,
                    sync_objects_.inFlightFence(frame_index_)) != VK_SUCCESS) {
    VKR_RENDER_ERROR("failed to submit draw command buffer");
  }
}

void Renderer::present(uint32_t imageIndex) {
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  VkSemaphore signalSemaphores[] = {
      sync_objects_.renderFinishedSemaphore(imageIndex)};

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
    VKR_RENDER_ERROR("failed to present swap chain image");
  }
}

} // namespace vkr::render
