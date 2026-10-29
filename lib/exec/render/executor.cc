#include "vkr/exec/render/executor.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/buffer.hh"
#include <algorithm>
#include <array>
#include <limits>

namespace vkr::exec {

RenderExecutor::RenderExecutor(const core::Device &device,
                               const core::Swapchain &swapchain,
                               const core::CommandPool &commandPool,
                               std::vector<core::CommandBuffer> &commandBuffers)
    : device_(device), swapchain_(swapchain), command_pool_(commandPool),
      command_buffers_(commandBuffers) {
  if (command_pool_.queueRole() != core::CommandQueueRole::Graphics) {
    VKR_EXEC_ERROR("RenderExecutor requires a graphics command pool");
  }

  if (command_buffers_.empty() ||
      command_buffers_.size() > std::numeric_limits<uint32_t>::max()) {
    VKR_EXEC_ERROR("RenderExecutor requires initialized command buffers");
  }
  for (const auto &buffer : command_buffers_) {
    if (!buffer.valid()) {
      VKR_EXEC_ERROR("RenderExecutor received an invalid command buffer");
    }
  }

  if (swapchain_.imageCount() == 0) {
    VKR_EXEC_ERROR("RenderExecutor requires an initialized swapchain");
  }

  image_available_.reserve(framesInFlight());
  in_flight_.reserve(framesInFlight());
  render_finished_.reserve(swapchain_.imageCount());
  pipelines_.resize(framesInFlight());
  pending_frames_.resize(framesInFlight(), false);

  for (uint32_t i = 0; i < framesInFlight(); ++i) {
    image_available_.emplace_back(device_);
    in_flight_.emplace_back(device_, true);
  }

  for (size_t i = 0; i < swapchain_.imageCount(); ++i) {
    render_finished_.emplace_back(device_);
  }
}

RenderExecutor::~RenderExecutor() {
  for (size_t frame = 0; frame < pending_frames_.size(); ++frame) {
    if (pending_frames_[frame]) {
      const auto fence = in_flight_[frame].fence();
      vkWaitForFences(device_.device(), 1, &fence, VK_TRUE, UINT64_MAX);
    }
  }
  // Presentation can still be waiting on render_finished_ after graphics ends.
  if (presentation_pending_) {
    vkQueueWaitIdle(device_.presentQueue());
  }
}

void RenderExecutor::wait() {
  ensureFrameInactive("wait");
  for (size_t frame = 0; frame < pending_frames_.size(); ++frame) {
    if (pending_frames_[frame]) {
      in_flight_[frame].wait();
      pending_frames_[frame] = false;
    }
    command_buffers_[frame].reset();
    pipelines_[frame].clear();
  }
}

auto RenderExecutor::beginFrame() -> bool {
  ensureFrameInactive("beginFrame");

  if (pending_frames_.at(current_frame_)) {
    in_flight_.at(current_frame_).wait();
    pending_frames_[current_frame_] = false;
  }

  auto &commands = command_buffers_.at(current_frame_);
  commands.reset();
  pipelines_.at(current_frame_).clear();

  uint32_t imageIndex = 0;
  if (!acquireNextImage(imageIndex)) {
    return false;
  }

  commands.begin();

  image_index_ = imageIndex;
  frame_index_ = current_frame_;

  frame_active_ = true;
  frame_submitted_ = false;
  frame_presented_ = false;

  if (profiler_) {
    profiler_->get().beginFrame(commandBuffer());
  }

  return true;
}

void RenderExecutor::submitFrame() {
  ensureFrameActive("submitFrame");

  if (frame_submitted_) {
    VKR_EXEC_ERROR("RenderExecutor::submitFrame called twice for one frame");
  }

  if (render_pass_active_) {
    VKR_EXEC_ERROR("RenderExecutor::submitFrame called inside a render pass");
  }

  if (profiler_) {
    profiler_->get().endFrame(commandBuffer());
  }

  command_buffers_.at(frame_index_).end();

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

  current_frame_ = (current_frame_ + 1) % framesInFlight();

  image_index_ = 0;
  frame_index_ = 0;
  frame_active_ = false;
  frame_submitted_ = false;
  frame_presented_ = false;
}

auto RenderExecutor::framesInFlight() const noexcept -> uint32_t {
  return static_cast<uint32_t>(command_buffers_.size());
}

void RenderExecutor::beginPass(const pipe::RenderPass &renderPass,
                               const Framebuffers &framebuffers,
                               const std::vector<VkClearValue> &clearValues,
                               uint32_t framebufferIndex,
                               VkSubpassContents contents) {
  ensureRecording("beginPass");
  if (render_pass_active_) {
    VKR_EXEC_ERROR("RenderExecutor::beginPass called inside a render pass");
  }

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

  vkCmdBeginRenderPass(commandBuffer(), &info, contents);
  render_pass_active_ = true;
}

void RenderExecutor::endPass() {
  ensureRecording("endPass");
  if (!render_pass_active_) {
    VKR_EXEC_ERROR("RenderExecutor::endPass called outside a render pass");
  }
  vkCmdEndRenderPass(commandBuffer());
  render_pass_active_ = false;
}

void RenderExecutor::barrier(VkPipelineStageFlags sourceStage,
                             VkAccessFlags sourceAccess,
                             VkPipelineStageFlags destinationStage,
                             VkAccessFlags destinationAccess) {
  ensureRecording("barrier");
  if (render_pass_active_) {
    VKR_EXEC_ERROR("RenderExecutor::barrier requires ending the render pass");
  }
  if (sourceStage == 0 || destinationStage == 0) {
    VKR_EXEC_ERROR("Barrier requires nonzero pipeline stages");
  }
  VkMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  barrier.srcAccessMask = sourceAccess;
  barrier.dstAccessMask = destinationAccess;
  vkCmdPipelineBarrier(commandBuffer(), sourceStage, destinationStage, 0, 1,
                       &barrier, 0, nullptr, 0, nullptr);
}

void RenderExecutor::barrier(const res::Buffer &buffer,
                             VkPipelineStageFlags sourceStage,
                             VkAccessFlags sourceAccess,
                             VkPipelineStageFlags destinationStage,
                             VkAccessFlags destinationAccess) {
  barrier(buffer.slice(0, buffer.size()), sourceStage, sourceAccess,
          destinationStage, destinationAccess);
}

void RenderExecutor::barrier(const res::BufferSlice &slice,
                             VkPipelineStageFlags sourceStage,
                             VkAccessFlags sourceAccess,
                             VkPipelineStageFlags destinationStage,
                             VkAccessFlags destinationAccess) {
  ensureRecording("barrier");
  if (render_pass_active_) {
    VKR_EXEC_ERROR("RenderExecutor::barrier requires ending the render pass");
  }
  if (!slice.valid() || sourceStage == 0 || destinationStage == 0) {
    VKR_EXEC_ERROR("Buffer barrier requires a valid range and nonzero stages");
  }
  VkBufferMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcAccessMask = sourceAccess;
  barrier.dstAccessMask = destinationAccess;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = slice.buffer();
  barrier.offset = slice.offset();
  barrier.size = slice.size();
  vkCmdPipelineBarrier(commandBuffer(), sourceStage, destinationStage, 0, 0,
                       nullptr, 1, &barrier, 0, nullptr);
}

void RenderExecutor::bindPipeline(const pipe::GraphicsPipeline &pipeline) {
  ensureRecording("bindPipeline");

  if (pipeline.pipeline() == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("bindPipeline received null VkPipeline");
  }

  if (pipeline.layout() == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("bindPipeline received null VkPipelineLayout");
  }

  auto &versions = pipelines_.at(frame_index_);
  const auto retained =
      std::find_if(versions.begin(), versions.end(),
                   [&pipeline](const auto &version) -> bool {
                     return version->pipeline() == pipeline.pipeline();
                   });
  if (retained == versions.end()) {
    versions.push_back(pipeline.version());
  }
  vkCmdBindPipeline(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.pipeline());
}

void RenderExecutor::bindPipeline(const pipe::GraphicsPipeline &pipeline,
                                  pipe::DescriptorSet &set, uint32_t setIndex) {
  if (setIndex >= pipeline.desc().layout.setLayouts.size()) {
    VKR_EXEC_ERROR("Descriptor set index {} is not declared in the pipeline",
                   setIndex);
  }
  bindPipeline(pipeline);
  set.update();
  const auto descriptorSet = set.set();
  vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.layout(), setIndex, 1, &descriptorSet, 0,
                          nullptr);
}

void RenderExecutor::bindPipeline(const pipe::GraphicsPipeline &pipeline,
                                  std::vector<pipe::DescriptorSet> &sets) {
  bindPipeline(pipeline);
  if (sets.size() > pipeline.desc().layout.setLayouts.size()) {
    VKR_EXEC_ERROR("Descriptor set count exceeds the pipeline layout");
  }
  bound_descriptors_.clear();
  for (auto &set : sets) {
    set.update();
    bound_descriptors_.push_back(set.set());
  }
  if (!bound_descriptors_.empty()) {
    vkCmdBindDescriptorSets(commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.layout(), 0,
                            static_cast<uint32_t>(bound_descriptors_.size()),
                            bound_descriptors_.data(), 0, nullptr);
  }
}

void RenderExecutor::setViewportAndScissor(VkExtent2D extent) {
  ensureRecording("setViewportAndScissor");

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(commandBuffer(), 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;

  vkCmdSetScissor(commandBuffer(), 0, 1, &scissor);
}

void RenderExecutor::drawIndexed(const scene::IVertexBuffer &vertexBuffer,
                                 const scene::IndexBuffer &indexBuffer) {
  ensureRecording("drawIndexed");

  if (vertexBuffer.vertexCount() == 0 || indexBuffer.indices().empty()) {
    return;
  }

  std::array<VkDeviceSize, 1> offsets = {0};
  std::array<VkBuffer, 1> vertexBuffers = {vertexBuffer.buffer()};

  vkCmdBindVertexBuffers(commandBuffer(), 0, 1, vertexBuffers.data(),
                         offsets.data());
  vkCmdBindIndexBuffer(commandBuffer(), indexBuffer.buffer(), 0,
                       VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(commandBuffer(),
                   static_cast<uint32_t>(indexBuffer.indices().size()), 1, 0, 0,
                   0);
}

void RenderExecutor::drawFullscreenTriangle() {
  ensureRecording("drawFullscreenTriangle");
  vkCmdDraw(commandBuffer(), 3, 1, 0, 0);
}

void RenderExecutor::beginProfileScope(std::string_view name) {
  ensureRecording("beginProfileScope");
  if (profiler_) {
    profiler_->get().beginScope(commandBuffer(), name);
  }
}

void RenderExecutor::endProfileScope() {
  ensureRecording("endProfileScope");
  if (profiler_) {
    profiler_->get().endScope(commandBuffer());
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

void RenderExecutor::ensureRecording(std::string_view op) const {
  ensureFrameActive(op);
  if (frame_submitted_) {
    VKR_EXEC_ERROR("RenderExecutor::{} called after submission", op);
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
  const auto commandBuffer = command_buffers_[frame_index_].buffer();
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
  submitInfo.pCommandBuffers = &commandBuffer;

  std::array<VkSemaphore, 1> signalSemaphores = {
      render_finished_.at(image_index_).semaphore()};

  submitInfo.signalSemaphoreCount = signalSemaphores.size();
  submitInfo.pSignalSemaphores = signalSemaphores.data();

  in_flight_.at(frame_index_).reset();
  if (vkQueueSubmit(command_pool_.queue(), 1, &submitInfo,
                    in_flight_.at(frame_index_).fence()) != VK_SUCCESS) {
    VKR_EXEC_ERROR("failed to submit draw command buffer");
  }
  pending_frames_[frame_index_] = true;
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

  presentation_pending_ = true;
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
