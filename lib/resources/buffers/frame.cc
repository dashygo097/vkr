#include "vkr/resources/buffers/frame_buffer.hh"

namespace vkr {

Framebuffers::Framebuffers(const Device &device, const RenderPass &renderPass,
                           const Swapchain &swapchain)
    : device(device), renderPass(renderPass), swapchain(swapchain) {
  create();
}

Framebuffers::~Framebuffers() { destroy(); }

void Framebuffers::create() {
  _framebuffers.resize(swapchain.imageViews().size());

  for (size_t i = 0; i < swapchain.imageViews().size(); i++) {
    VkImageView attachments[] = {swapchain.imageViews()[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass.renderPass();
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapchain.extent2D().width;
    framebufferInfo.height = swapchain.extent2D().height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr,
                            &_framebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void Framebuffers::destroy() {
  for (auto framebuffer : _framebuffers) {
    if (framebuffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
    }
  }
  _framebuffers.clear();
}
} // namespace vkr
