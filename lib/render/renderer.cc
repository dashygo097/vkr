#include "vkr/render/renderer.hh"
#include "vkr/logger.hh"

namespace vkr::render {

Renderer::Renderer(const core::Device &device, const core::Swapchain &swapchain,
                   const core::CommandPool &commandPool,
                   core::SyncObjects &syncObjects,
                   resource::ResourceManager &resourceManager)
    : device_(device), swapchain_(swapchain), command_pool_(commandPool),
      sync_objects_(syncObjects), resource_manager_(resourceManager) {
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

void Renderer::drawIndexed(const resource::IVertexBuffer &vertexBuffer,
                           const resource::IndexBuffer &indexBuffer) {
  ensureFrameActive("drawIndexed");

  if (vertexBuffer.vertexCount() == 0 || indexBuffer.indices().empty()) {
    return;
  }

  VkDeviceSize offsets[] = {0};
  VkBuffer vertexBuffers[] = {vertexBuffer.buffer()};

  vkCmdBindVertexBuffers(command_buffer_, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(command_buffer_, indexBuffer.buffer(), 0,
                       VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(command_buffer_,
                   static_cast<uint32_t>(indexBuffer.indices().size()), 1, 0, 0,
                   0);
}

void Renderer::drawGeometry() {
  ensureFrameActive("drawGeometry");

  auto meshNames = resource_manager_.listMeshNames();

  if (meshNames.empty()) {
    vkCmdDraw(command_buffer_, 3, 1, 0, 0);
    return;
  }

  for (const auto &name : meshNames) {
    auto mesh = resource_manager_.getMesh(name);
    if (!mesh || !mesh->isValid()) {
      continue;
    }

    auto vertexBuffer = mesh->vertexBufferBase();
    auto indexBuffer = mesh->indexBuffer();
    if (!vertexBuffer || !indexBuffer) {
      continue;
    }

    drawIndexed(*vertexBuffer, *indexBuffer);
  }
}

void Renderer::drawFullscreenTriangle() {
  ensureFrameActive("drawFullscreenTriangle");
  vkCmdDraw(command_buffer_, 3, 1, 0, 0);
}

void Renderer::drawUI(ui::UI &ui) {
  ensureFrameActive("drawUI");
  ui.render(command_buffer_);
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
    swapchain_out_of_date_ = true;
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
    swapchain_out_of_date_ = true;
    return;
  }

  if (result != VK_SUCCESS) {
    VKR_RENDER_ERROR("failed to present swap chain image");
  }
}

} // namespace vkr::render
