#include "vkr/exec/render/executor.hh"
#include "vkr/logger.hh"

namespace vkr::exec {

RenderExecutor::RenderExecutor(const core::Device &device,
                               const core::Swapchain &swapchain,
                               const core::CommandPool &commandPool,
                               scene::Scene &scene,
                               core::CommandBuffers &commandBuffers)
    : device_(device), swapchain_(swapchain), command_pool_(commandPool),
      scene_(scene), command_buffers_(commandBuffers) {
  if (command_pool_.queueRole() != core::CommandQueueRole::Graphics) {
    VKR_EXEC_ERROR("RenderExecutor requires a graphics command pool");
  }

  if (command_buffers_.empty()) {
    VKR_EXEC_ERROR("RenderExecutor requires initialized command buffers");
  }

  if (swapchain_.imageCount() == 0) {
    VKR_EXEC_ERROR("RenderExecutor requires an initialized swapchain");
  }

  image_available_.reserve(framesInFlight());
  in_flight_.reserve(framesInFlight());
  render_finished_.reserve(swapchain_.imageCount());

  for (uint32_t i = 0; i < framesInFlight(); ++i) {
    image_available_.emplace_back(device_);
    in_flight_.emplace_back(device_, true);
  }

  for (size_t i = 0; i < swapchain_.imageCount(); ++i) {
    render_finished_.emplace_back(device_);
  }
}

auto RenderExecutor::beginFrame() -> bool {
  ensureFrameInactive("beginFrame");

  in_flight_.at(current_frame_).wait();

  uint32_t imageIndex = 0;
  if (!acquireNextImage(imageIndex)) {
    return false;
  }

  in_flight_.at(current_frame_).reset();

  VkCommandBuffer commandBuffer = command_buffers_.buffer(current_frame_);
  vkResetCommandBuffer(commandBuffer, 0);

  image_index_ = imageIndex;
  frame_index_ = current_frame_;
  command_buffer_ = commandBuffer;

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(command_buffer_, &beginInfo) != VK_SUCCESS) {
    VKR_EXEC_ERROR("failed to begin recording command buffer");
  }

  frame_active_ = true;
  frame_submitted_ = false;
  frame_presented_ = false;

  if (profiler_) {
    profiler_->get().beginFrame(command_buffer_);
  }

  return true;
}

void RenderExecutor::submitFrame() {
  ensureFrameActive("submitFrame");

  if (frame_submitted_) {
    VKR_EXEC_ERROR("RenderExecutor::submitFrame called twice for one frame");
  }

  if (profiler_) {
    profiler_->get().endFrame(command_buffer_);
  }

  if (vkEndCommandBuffer(command_buffer_) != VK_SUCCESS) {
    VKR_EXEC_ERROR("failed to record command buffer");
  }

  submitCommandBuffer();
  frame_submitted_ = true;
}

void RenderExecutor::presentFrame() {
  ensureFrameActive("presentFrame");

  if (!frame_submitted_) {
    VKR_EXEC_ERROR("RenderExecutor::presentFrame called before submitFrame");
  }

  if (frame_presented_) {
    VKR_EXEC_ERROR("RenderExecutor::presentFrame called twice for one frame");
  }

  present(image_index_);
  frame_presented_ = true;
}

void RenderExecutor::setProfiler(Profiler &profiler) noexcept {
  profiler_ = profiler;
}

void RenderExecutor::clearProfiler() noexcept { profiler_.reset(); }

void RenderExecutor::endFrame() {
  ensureFrameActive("endFrame");

  if (!frame_submitted_) {
    VKR_EXEC_ERROR("RenderExecutor::endFrame called before submitFrame");
  }

  current_frame_ = (current_frame_ + 1) % command_buffers_.size();

  image_index_ = 0;
  frame_index_ = 0;
  command_buffer_ = VK_NULL_HANDLE;
  frame_active_ = false;
  frame_submitted_ = false;
  frame_presented_ = false;
}

auto RenderExecutor::framesInFlight() const noexcept -> uint32_t {
  return command_buffers_.size();
}

void RenderExecutor::beginPass(const pipeline::RenderPass &renderPass,
                               const Framebuffers &framebuffers,
                               const std::vector<VkClearValue> &clearValues,
                               uint32_t framebufferIndex,
                               VkSubpassContents contents) {
  ensureFrameActive("beginPass");

  if (framebufferIndex >= framebuffers.buffers().size()) {
    VKR_EXEC_ERROR("Framebuffer index {} out of range, framebuffer count {}",
                   framebufferIndex, framebuffers.buffers().size());
  }

  const auto extent = framebuffers.extent();
  if (extent.width == 0 || extent.height == 0) {
    VKR_EXEC_ERROR("Framebuffers has invalid extent: {}x{}", extent.width,
                   extent.height);
  }

  VkRenderPassBeginInfo info{};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  info.renderPass = renderPass.renderPass();
  info.framebuffer = framebuffers.buffer(framebufferIndex);
  info.renderArea = {.offset = {0, 0}, .extent = extent};
  info.clearValueCount = static_cast<uint32_t>(clearValues.size());
  info.pClearValues = clearValues.empty() ? nullptr : clearValues.data();

  vkCmdBeginRenderPass(command_buffer_, &info, contents);
}

void RenderExecutor::endPass() {
  ensureFrameActive("endPass");
  vkCmdEndRenderPass(command_buffer_);
}

void RenderExecutor::bindPipeline(const pipeline::GraphicsPipeline &pipeline) {
  ensureFrameActive("bindPipeline");

  if (pipeline.pipeline() == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("bindPipeline received null VkPipeline");
  }

  if (pipeline.layout() == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("bindPipeline received null VkPipelineLayout");
  }

  vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.pipeline());
}

void RenderExecutor::bindPipeline(const pipeline::GraphicsPipeline &pipeline,
                                  const pipeline::DescriptorSets &sets) {
  bindPipeline(pipeline);

  if (sets.empty()) {
    return;
  }

  if (frame_index_ >= sets.count()) {
    VKR_EXEC_ERROR("Descriptor set frame index {} out of range, count {}",
                   frame_index_, sets.count());
  }

  VkDescriptorSet descriptorSet = sets.set(frame_index_);

  vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.layout(), 0, 1, &descriptorSet, 0, nullptr);
}

void RenderExecutor::setViewportAndScissor(VkExtent2D extent) {
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

void RenderExecutor::drawIndexed(const scene::IVertexBuffer &vertexBuffer,
                                 const scene::IndexBuffer &indexBuffer) {
  ensureFrameActive("drawIndexed");

  if (vertexBuffer.vertexCount() == 0 || indexBuffer.indices().empty()) {
    return;
  }

  std::array<VkDeviceSize, 1> offsets = {0};
  std::array<VkBuffer, 1> vertexBuffers = {vertexBuffer.buffer()};

  vkCmdBindVertexBuffers(command_buffer_, 0, 1, vertexBuffers.data(),
                         offsets.data());
  vkCmdBindIndexBuffer(command_buffer_, indexBuffer.buffer(), 0,
                       VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(command_buffer_,
                   static_cast<uint32_t>(indexBuffer.indices().size()), 1, 0, 0,
                   0);
}

void RenderExecutor::drawGeometry() {
  ensureFrameActive("drawGeometry");

  auto meshNames = scene_.listMeshNames();

  if (meshNames.empty()) {
    vkCmdDraw(command_buffer_, 3, 1, 0, 0);
    return;
  }

  for (const auto &name : meshNames) {
    const auto mesh = scene_.findMesh(name);
    if (!mesh || !mesh->get().isValid()) {
      continue;
    }

    const auto vertexBuffer = mesh->get().vertexBufferBase();
    const auto indexBuffer = mesh->get().indexBuffer();
    if (!vertexBuffer || !indexBuffer) {
      continue;
    }

    drawIndexed(vertexBuffer->get(), indexBuffer->get());
  }
}

void RenderExecutor::drawFullscreenTriangle() {
  ensureFrameActive("drawFullscreenTriangle");
  vkCmdDraw(command_buffer_, 3, 1, 0, 0);
}

void RenderExecutor::beginProfileScope(std::string_view name) {
  ensureFrameActive("beginProfileScope");
  if (profiler_) {
    profiler_->get().beginScope(command_buffer_, name);
  }
}

void RenderExecutor::endProfileScope() {
  ensureFrameActive("endProfileScope");
  if (profiler_) {
    profiler_->get().endScope(command_buffer_);
  }
}

void RenderExecutor::ensureFrameActive(std::string_view op) const {
  if (!frame_active_) {
    VKR_EXEC_ERROR("RenderExecutor::{} called without an active frame", op);
  }
}

void RenderExecutor::ensureFrameInactive(std::string_view op) const {
  if (frame_active_) {
    VKR_EXEC_ERROR("RenderExecutor::{} called while a frame is already active",
                   op);
  }
}

auto RenderExecutor::acquireNextImage(uint32_t &imageIndex) -> bool {
  VkResult result = vkAcquireNextImageKHR(
      device_.device(), swapchain_.swapchain(), UINT64_MAX,
      image_available_.at(current_frame_).semaphore(), VK_NULL_HANDLE,
      &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    swapchain_out_of_date_ = true;
    return false;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    VKR_EXEC_ERROR("failed to acquire swap chain image");
  }

  return true;
}

void RenderExecutor::submitCommandBuffer() {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  std::array<VkSemaphore, 1> waitSemaphores = {
      image_available_.at(frame_index_).semaphore()};

  std::array<VkPipelineStageFlags, 1> waitStages = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  submitInfo.waitSemaphoreCount = waitSemaphores.size();
  submitInfo.pWaitSemaphores = waitSemaphores.data();
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &command_buffer_;

  std::array<VkSemaphore, 1> signalSemaphores = {
      render_finished_.at(image_index_).semaphore()};

  submitInfo.signalSemaphoreCount = signalSemaphores.size();
  submitInfo.pSignalSemaphores = signalSemaphores.data();

  if (vkQueueSubmit(command_pool_.queue(), 1, &submitInfo,
                    in_flight_.at(frame_index_).fence()) != VK_SUCCESS) {
    VKR_EXEC_ERROR("failed to submit draw command buffer");
  }
}

void RenderExecutor::present(uint32_t imageIndex) {
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  std::array<VkSemaphore, 1> signalSemaphores = {
      render_finished_.at(imageIndex).semaphore()};

  presentInfo.waitSemaphoreCount = signalSemaphores.size();
  presentInfo.pWaitSemaphores = signalSemaphores.data();

  std::array<VkSwapchainKHR, 1> swapchains = {swapchain_.swapchain()};
  presentInfo.swapchainCount = swapchains.size();
  presentInfo.pSwapchains = swapchains.data();
  presentInfo.pImageIndices = &imageIndex;

  VkResult result = vkQueuePresentKHR(device_.presentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    swapchain_out_of_date_ = true;
    return;
  }

  if (result != VK_SUCCESS) {
    VKR_EXEC_ERROR("failed to present swap chain image");
  }
}

} // namespace vkr::exec
