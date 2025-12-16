#include "vkr/core/command_buffer.hh"

namespace vkr {

CommandBuffers::CommandBuffers(const Device &device,
                               const CommandPool &commandPool)
    : device(device), commandPool(commandPool) {
  _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool.commandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

  if (vkAllocateCommandBuffers(device.device(), &allocInfo,
                               _commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

CommandBuffers::~CommandBuffers() {
  if (!_commandBuffers.empty()) {
    vkFreeCommandBuffers(device.device(), commandPool.commandPool(),
                         static_cast<uint32_t>(_commandBuffers.size()),
                         _commandBuffers.data());
  }
  _commandBuffers.clear();
}

void CommandBuffers::record(
    uint32_t imageIndex, uint32_t currentFrame, VkRenderPass renderPass,
    VkPipelineLayout pipelineLayout,
    const std::vector<VkDescriptorSet> &descriptorSets,
    const std::vector<VkFramebuffer> &framebuffers, VkExtent2D extent,
    VkPipeline graphicsPipeline,
    const std::vector<std::shared_ptr<VertexBuffer>> &vertexBuffers,
    const std::vector<std::shared_ptr<IndexBuffer>> &indexBuffers, UI &ui) {
  beginRecording(currentFrame);
  beginRenderPass(currentFrame, renderPass, framebuffers[imageIndex], extent);
  vkCmdBindPipeline(_commandBuffers[currentFrame],
                    VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
  setViewportAndScissor(currentFrame, extent);

  for (size_t i = 0; i < vertexBuffers.size() && i < indexBuffers.size(); ++i) {
    if (vertexBuffers[i] && indexBuffers[i] &&
        vertexBuffers[i]->buffer() != VK_NULL_HANDLE &&
        indexBuffers[i]->buffer() != VK_NULL_HANDLE &&
        !vertexBuffers[i]->vertices().empty() &&
        !indexBuffers[i]->indices().empty()) {
      drawIndexed(currentFrame, pipelineLayout, descriptorSets[currentFrame],
                  vertexBuffers[i], indexBuffers[i]);
    }
  }

  ui.render(_commandBuffers[currentFrame]);
  endRenderPass(currentFrame);
  endRecording(currentFrame);
}

void CommandBuffers::beginRecording(uint16_t index) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(_commandBuffers[index], &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }
}

void CommandBuffers::beginRenderPass(uint16_t index, VkRenderPass renderPass,
                                     VkFramebuffer framebuffer,
                                     VkExtent2D extent) {
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = framebuffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = extent;

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(_commandBuffers[index], &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffers::setViewportAndScissor(uint16_t index, VkExtent2D extent) {
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(_commandBuffers[index], 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;
  vkCmdSetScissor(_commandBuffers[index], 0, 1, &scissor);
}

void CommandBuffers::drawIndexed(
    uint16_t index, VkPipelineLayout pipelineLayout,
    VkDescriptorSet descriptorSet,
    const std::shared_ptr<VertexBuffer> &vertexBuffer,
    const std::shared_ptr<IndexBuffer> &indexBuffer) {
  VkBuffer vb = vertexBuffer->buffer();
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(_commandBuffers[index], 0, 1, &vb, offsets);

  vkCmdBindIndexBuffer(_commandBuffers[index], indexBuffer->buffer(), 0,
                       VK_INDEX_TYPE_UINT16);

  vkCmdBindDescriptorSets(_commandBuffers[index],
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &descriptorSet, 0, nullptr);

  vkCmdDrawIndexed(_commandBuffers[index],
                   static_cast<uint32_t>(indexBuffer->indices().size()), 1, 0,
                   0, 0);
}

void CommandBuffers::endRenderPass(uint16_t index) {
  vkCmdEndRenderPass(_commandBuffers[index]);
}

void CommandBuffers::endRecording(uint16_t index) {
  if (vkEndCommandBuffer(_commandBuffers[index]) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

} // namespace vkr
