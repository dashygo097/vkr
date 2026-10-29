#include "vkr/exec/compute/executor.hh"
#include "vkr/exec/profiler.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/buffer.hh"

namespace vkr::exec {

ComputeExecutor::ComputeExecutor(const core::Device &device,
                                 const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool),
      command_buffer_(commandPool.allocate()) {
  if (!device_.supportsCompute()) {
    VKR_EXEC_ERROR("ComputeExecutor requires compute queue support");
  }

  if (command_pool_.queueFamily() != device_.computeFamily()) {
    VKR_EXEC_ERROR("ComputeExecutor command pool queue family ({}) does not "
                   "match device compute queue family ({})",
                   command_pool_.queueFamily(), device_.computeFamily());
  }
}

void ComputeExecutor::begin() {
  ensureInactive("begin");

  command_buffer_.reset();
  command_buffer_.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  active_ = true;
  submitted_ = false;

  if (profiler_) {
    profiler_->get().beginFrame(command_buffer_.buffer());
  }
}

void ComputeExecutor::submitAndWait() {
  ensureActive("submitAndWait");

  if (submitted_) {
    VKR_EXEC_ERROR("ComputeExecutor::submitAndWait called twice");
  }

  if (profiler_) {
    profiler_->get().endFrame(command_buffer_.buffer());
  }

  command_buffer_.end();

  core::Fence fence{device_};
  const auto commandBuffer = command_buffer_.buffer();

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(command_pool_.queue(), 1, &submitInfo, fence.fence()) !=
      VK_SUCCESS) {
    VKR_EXEC_ERROR("failed to submit compute command buffer");
  }

  fence.wait();
  submitted_ = true;
}

void ComputeExecutor::end() {
  ensureActive("end");

  if (!submitted_) {
    VKR_EXEC_ERROR("ComputeExecutor::end called before submitAndWait");
  }

  active_ = false;
  submitted_ = false;
}

void ComputeExecutor::setProfiler(Profiler &profiler) noexcept {
  profiler_ = profiler;
}

void ComputeExecutor::clearProfiler() noexcept { profiler_.reset(); }

auto ComputeExecutor::commandBuffer() const -> VkCommandBuffer {
  ensureRecording("commandBuffer");
  return command_buffer_.buffer();
}

void ComputeExecutor::bindPipeline(const pipe::ComputePipeline &pipeline) {
  ensureRecording("bindPipeline");

  if (pipeline.pipeline() == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("bindPipeline received null VkPipeline");
  }

  if (pipeline.layout() == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("bindPipeline received null VkPipelineLayout");
  }

  vkCmdBindPipeline(command_buffer_.buffer(), VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline.pipeline());
}

void ComputeExecutor::bindPipeline(const pipe::ComputePipeline &pipeline,
                                   pipe::DescriptorSet &set,
                                   uint32_t setIndex) {
  if (setIndex >= pipeline.desc().layout.setLayouts.size()) {
    VKR_EXEC_ERROR("Descriptor set index {} is not declared in the pipeline",
                   setIndex);
  }
  bindPipeline(pipeline);
  set.update();
  const auto descriptorSet = set.set();
  vkCmdBindDescriptorSets(command_buffer_.buffer(),
                          VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout(),
                          setIndex, 1, &descriptorSet, 0, nullptr);
}

void ComputeExecutor::bindPipeline(const pipe::ComputePipeline &pipeline,
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
    vkCmdBindDescriptorSets(command_buffer_.buffer(),
                            VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout(),
                            0, static_cast<uint32_t>(bound_descriptors_.size()),
                            bound_descriptors_.data(), 0, nullptr);
  }
}

void ComputeExecutor::dispatch(uint32_t groupCountX, uint32_t groupCountY,
                               uint32_t groupCountZ) {
  ensureRecording("dispatch");

  if (groupCountX == 0 || groupCountY == 0 || groupCountZ == 0) {
    VKR_EXEC_ERROR("dispatch has invalid group count: {}x{}x{}", groupCountX,
                   groupCountY, groupCountZ);
  }

  vkCmdDispatch(command_buffer_.buffer(), groupCountX, groupCountY,
                groupCountZ);
}

void ComputeExecutor::hostRead(const res::Buffer &buffer) {
  hostRead(buffer.slice(0, buffer.size()));
}

void ComputeExecutor::hostRead(const res::BufferSlice &slice) {
  ensureRecording("hostRead");
  if (!slice.valid() || !slice.hostVisible()) {
    VKR_EXEC_ERROR("hostRead requires a valid host-visible buffer slice");
  }
  VkBufferMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = slice.buffer();
  barrier.offset = slice.offset();
  barrier.size = slice.size();

  vkCmdPipelineBarrier(
      command_buffer_.buffer(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

void ComputeExecutor::beginProfileScope(std::string_view name) {
  ensureRecording("beginProfileScope");
  if (profiler_) {
    profiler_->get().beginScope(command_buffer_.buffer(), name);
  }
}

void ComputeExecutor::endProfileScope() {
  ensureRecording("endProfileScope");
  if (profiler_) {
    profiler_->get().endScope(command_buffer_.buffer());
  }
}

void ComputeExecutor::ensureRecording(std::string_view op) const {
  ensureActive(op);
  if (submitted_) {
    VKR_EXEC_ERROR("ComputeExecutor::{} called after submission", op);
  }
}

void ComputeExecutor::ensureActive(std::string_view op) const {
  if (!active_) {
    VKR_EXEC_ERROR("ComputeExecutor::{} called without an active command "
                   "buffer",
                   op);
  }
}

void ComputeExecutor::ensureInactive(std::string_view op) const {
  if (active_) {
    VKR_EXEC_ERROR("ComputeExecutor::{} called while a command buffer is "
                   "active",
                   op);
  }
}

} // namespace vkr::exec
