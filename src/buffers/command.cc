#include "vkr/buffers/command.hpp"

namespace vkr {

CommandBuffers::CommandBuffers(VkDevice device, VkCommandPool commandPool)
    : device(device), commandPool(commandPool) {
  commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

CommandBuffers::CommandBuffers(const VulkanContext &ctx)
    : CommandBuffers(ctx.device, ctx.commandPool) {}

CommandBuffers::~CommandBuffers() {
  if (!commandBuffers.empty()) {
    vkFreeCommandBuffers(device, commandPool,
                         static_cast<uint32_t>(commandBuffers.size()),
                         commandBuffers.data());
  }
  commandBuffers.clear();
}

void recordCommandBuffer(uint32_t imageIndex, uint32_t currentFrame,
                         std::vector<Vertex> vertices,
                         std::vector<uint16_t> indices, VkBuffer indexBuffer,
                         VkBuffer vertexBuffer, VkCommandBuffer commandBuffer,
                         VkRenderPass renderPass,
                         VkPipelineLayout pipelineLayout,
                         std::vector<VkDescriptorSet> descriptorSets,
                         std::vector<VkFramebuffer> swapchainFrameBuffers,
                         VkExtent2D swapchainExtent,
                         VkPipeline graphicsPipeline) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapchainFrameBuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapchainExtent;

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapchainExtent.width;
  viewport.height = (float)swapchainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  if (vertexBuffer != VK_NULL_HANDLE && indexBuffer != VK_NULL_HANDLE &&
      !vertices.empty() && !indices.empty()) {
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &descriptorSets[currentFrame],
                            0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0,
                     0, 0);
  }

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void recordCommandBuffer(
    uint32_t imageIndex, uint32_t currentFrame, VkCommandBuffer commandBuffer,
    VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
    std::vector<VkDescriptorSet> descriptorSets,
    std::vector<VkFramebuffer> swapchainFrameBuffers,
    VkExtent2D swapchainExtent, VkPipeline graphicsPipeline,
    const std::vector<std::unique_ptr<VertexBuffer>> &vertexBuffers,
    const std::vector<std::unique_ptr<IndexBuffer>> &indexBuffers, UI &ui) {

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapchainFrameBuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapchainExtent;
  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapchainExtent.width;
  viewport.height = (float)swapchainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  for (size_t i = 0; i < vertexBuffers.size() && i < indexBuffers.size(); i++) {
    if (vertexBuffers[i] && indexBuffers[i] &&
        vertexBuffers[i]->getVkBuffer() != VK_NULL_HANDLE &&
        indexBuffers[i]->getVkBuffer() != VK_NULL_HANDLE &&
        !vertexBuffers[i]->getVertices().empty() &&
        !indexBuffers[i]->getIndices().empty()) {

      VkBuffer vertexBuffer = vertexBuffers[i]->getVkBuffer();
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

      vkCmdBindIndexBuffer(commandBuffer, indexBuffers[i]->getVkBuffer(), 0,
                           VK_INDEX_TYPE_UINT16);

      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipelineLayout, 0, 1,
                              &descriptorSets[currentFrame], 0, nullptr);

      vkCmdDrawIndexed(
          commandBuffer,
          static_cast<uint32_t>(indexBuffers[i]->getIndices().size()), 1, 0, 0,
          0);
    }
  }

  ui.render(commandBuffer);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}
} // namespace vkr
