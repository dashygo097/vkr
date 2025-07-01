#include "interface/frame_buffers.hpp"

FrameBuffers::FrameBuffers(VkDevice device, VkRenderPass renderPass,
                           std::vector<VkImageView> imageViews,
                           VkExtent2D extend)
    : device(device), renderPass(renderPass), swapChainImageViews(imageViews),
      swapChainExtent(extend) {
  create();
}

FrameBuffers::~FrameBuffers() { destroy(); }

void FrameBuffers::create() {
  frameBuffers.resize(swapChainImageViews.size());

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    VkImageView attachments[] = {swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &frameBuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void FrameBuffers::destroy() {
  for (auto framebuffer : frameBuffers) {
    if (framebuffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
  }
  frameBuffers.clear();
}
