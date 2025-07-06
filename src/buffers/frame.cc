#include "vkr/buffers/frame.hpp"

Framebuffers::Framebuffers(VkDevice device, VkRenderPass renderPass,
                           std::vector<VkImageView> swapchainImageViews,
                           VkExtent2D swapchainExtend)
    : device(device), renderPass(renderPass),
      swapchainImageViews(swapchainImageViews),
      swapchainExtent(swapchainExtend) {
  create();
}

Framebuffers::Framebuffers(const VulkanContext &ctx)
    : device(ctx.device), renderPass(ctx.renderPass),
      swapchainImageViews(ctx.swapchainImageViews),
      swapchainExtent(ctx.swapchainExtent) {
  create();
}

Framebuffers::~Framebuffers() { destroy(); }

void Framebuffers::create() {
  framebuffers.resize(swapchainImageViews.size());

  for (size_t i = 0; i < swapchainImageViews.size(); i++) {
    VkImageView attachments[] = {swapchainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapchainExtent.width;
    framebufferInfo.height = swapchainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &framebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void Framebuffers::destroy() {
  for (auto framebuffer : framebuffers) {
    if (framebuffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
  }
  framebuffers.clear();
}
