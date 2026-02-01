#include "vkr/resources/buffers/frame_buffer.hh"
#include "vkr/logger.hh"

namespace vkr::resource {

Framebuffers::Framebuffers(const core::Device &device,
                           const core::Swapchain &swapchain,
                           const DepthResources &depthResources,
                           const pipeline::RenderPass &renderPass)
    : device_(device), swapchain_(swapchain), depth_resources_(depthResources),
      render_pass_(renderPass) {
  create();
}

Framebuffers::~Framebuffers() { destroy(); }

void Framebuffers::create() {
  vk_framebuffers_.resize(swapchain_.imageViews().size());

  for (size_t i = 0; i < swapchain_.imageViews().size(); i++) {
    std::array<VkImageView, 2> attachments = {swapchain_.imageViews()[i],
                                              depth_resources_.imageView()};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = render_pass_.renderPass();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapchain_.extent2D().width;
    framebufferInfo.height = swapchain_.extent2D().height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device_.device(), &framebufferInfo, nullptr,
                            &vk_framebuffers_[i]) != VK_SUCCESS) {
      VKR_RES_ERROR("Failed to create framebuffer!");
    }
  }
}

void Framebuffers::destroy() {
  for (auto framebuffer : vk_framebuffers_) {
    if (framebuffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device_.device(), framebuffer, nullptr);
    }
  }
  vk_framebuffers_.clear();
}
} // namespace vkr::resource
