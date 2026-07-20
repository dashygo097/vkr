#include "vkr/exec/compute/executor.hh"
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
}

void ComputeExecutor::submitAndWait() {
  ensureActive("submitAndWait");

  if (submitted_) {
    VKR_EXEC_ERROR("ComputeExecutor::submitAndWait called twice");
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

auto ComputeExecutor::commandBuffer() const -> VkCommandBuffer {
  ensureActive("commandBuffer");
  return command_buffer_;
}

void ComputeExecutor::bindComputePipeline(
    VkPipeline pipeline, VkPipelineLayout pipelineLayout,
    const std::vector<VkDescriptorSet> &descriptorSets) {
  ensureActive("bindComputePipeline");

  if (pipeline == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("bindComputePipeline received null VkPipeline");
  }

  if (pipelineLayout == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("bindComputePipeline received null VkPipelineLayout");
  }

  vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  if (descriptorSets.empty()) {
    return;
  }

  VkDescriptorSet descriptorSet = descriptorSets[0];
  vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
}

void ComputeExecutor::dispatch(uint32_t groupCountX, uint32_t groupCountY,
                               uint32_t groupCountZ) {
  ensureActive("dispatch");

  if (groupCountX == 0 || groupCountY == 0 || groupCountZ == 0) {
    VKR_EXEC_ERROR("dispatch has invalid group count: {}x{}x{}",
                   groupCountX, groupCountY, groupCountZ);
  }

  vkCmdDispatch(command_buffer_, groupCountX, groupCountY, groupCountZ);
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

void ComputeExecutor::ensureActive(const char *op) const {
  if (!active_) {
    VKR_EXEC_ERROR("ComputeExecutor::{} called without an active command "
                   "buffer",
                   op);
  }
}

void ComputeExecutor::ensureInactive(const char *op) const {
  if (active_) {
    VKR_EXEC_ERROR("ComputeExecutor::{} called while a command buffer is "
                   "active",
                   op);
  }
}

} // namespace vkr::exec
