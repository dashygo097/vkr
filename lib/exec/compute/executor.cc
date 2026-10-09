#include "vkr/exec/compute/executor.hh"
#include "vkr/exec/profiler.hh"
#include "vkr/logger.hh"

namespace vkr::exec {

ComputeExecutor::ComputeExecutor(const core::Device &device,
                                 const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool) {
  if (!device_.supportsCompute()) {
    VKR_EXEC_ERROR("ComputeExecutor requires compute queue support");
  }

  if (command_pool_.queueFamily() != device_.computeFamily()) {
    VKR_EXEC_ERROR("ComputeExecutor command pool queue family ({}) does not "
                   "match device compute queue family ({})",
                   command_pool_.queueFamily(), device_.computeFamily());
  }

  allocateCommandBuffer();
}

ComputeExecutor::~ComputeExecutor() { freeCommandBuffer(); }

void ComputeExecutor::begin() {
  ensureInactive("begin");

  vkResetCommandBuffer(command_buffer_, 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(command_buffer_, &beginInfo) != VK_SUCCESS) {
    VKR_EXEC_ERROR("failed to begin compute command buffer");
  }

  active_ = true;
  submitted_ = false;

  if (profiler_) {
    profiler_->get().beginFrame(command_buffer_);
  }
}

void ComputeExecutor::submitAndWait() {
  ensureActive("submitAndWait");

  if (submitted_) {
    VKR_EXEC_ERROR("ComputeExecutor::submitAndWait called twice");
  }

  if (profiler_) {
    profiler_->get().endFrame(command_buffer_);
  }

  if (vkEndCommandBuffer(command_buffer_) != VK_SUCCESS) {
    VKR_EXEC_ERROR("failed to end compute command buffer");
  }

  core::Fence fence{device_};

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &command_buffer_;

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
  ensureActive("commandBuffer");
  return command_buffer_;
}

void ComputeExecutor::bindPipeline(const pipeline::ComputePipeline &pipeline) {
  ensureActive("bindPipeline");

  if (pipeline.pipeline() == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("bindPipeline received null VkPipeline");
  }

  if (pipeline.layout() == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("bindPipeline received null VkPipelineLayout");
  }

  vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline.pipeline());
}

void ComputeExecutor::bindPipeline(const pipeline::ComputePipeline &pipeline,
                                  pipeline::DescriptorSet &set,
                                  uint32_t setIndex) {
  if (setIndex >= pipeline.desc().layout.setLayouts.size()) {
    VKR_EXEC_ERROR("Descriptor set index {} is not declared in the pipeline",
                   setIndex);
  }
  bindPipeline(pipeline);
  set.update();
  const auto descriptorSet = set.set();
  vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipeline.layout(), setIndex, 1, &descriptorSet, 0,
                          nullptr);
}

void ComputeExecutor::bindPipeline(const pipeline::ComputePipeline &pipeline,
                                  std::vector<pipeline::DescriptorSet> &sets) {
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
    vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipeline.layout(), 0,
                            static_cast<uint32_t>(bound_descriptors_.size()),
                            bound_descriptors_.data(), 0, nullptr);
  }
}

void ComputeExecutor::dispatch(uint32_t groupCountX, uint32_t groupCountY,
                               uint32_t groupCountZ) {
  ensureActive("dispatch");

  if (groupCountX == 0 || groupCountY == 0 || groupCountZ == 0) {
    VKR_EXEC_ERROR("dispatch has invalid group count: {}x{}x{}", groupCountX,
                   groupCountY, groupCountZ);
  }

  vkCmdDispatch(command_buffer_, groupCountX, groupCountY, groupCountZ);
}

void ComputeExecutor::beginProfileScope(std::string_view name) {
  ensureActive("beginProfileScope");
  if (profiler_) {
    profiler_->get().beginScope(command_buffer_, name);
  }
}

void ComputeExecutor::endProfileScope() {
  ensureActive("endProfileScope");
  if (profiler_) {
    profiler_->get().endScope(command_buffer_);
  }
}

void ComputeExecutor::allocateCommandBuffer() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = command_pool_.commandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(device_.device(), &allocInfo,
                               &command_buffer_) != VK_SUCCESS) {
    VKR_EXEC_ERROR("failed to allocate compute command buffer");
  }
}

void ComputeExecutor::freeCommandBuffer() noexcept {
  if (command_buffer_ != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(device_.device(), command_pool_.commandPool(), 1,
                         &command_buffer_);
    command_buffer_ = VK_NULL_HANDLE;
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
