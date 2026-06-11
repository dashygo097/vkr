#include "vkr/resource/buffers/frame_buffer.hh"
#include "vkr/logger.hh"

namespace vkr::resource {

FramebufferSet::FramebufferSet(const core::Device &device,
                               const pipeline::RenderPass &renderPass)
    : device_(device), render_pass_(renderPass) {}

FramebufferSet::~FramebufferSet() { destroy(); }

void FramebufferSet::create() {
  vk_framebuffers_.resize(desc_.attachments.size(), VK_NULL_HANDLE);

  for (size_t i = 0; i < desc_.attachments.size(); i++) {
    const auto &attachments = desc_.attachments[i];

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = render_pass_.renderPass();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = desc_.extent.width;
    framebufferInfo.height = desc_.extent.height;
    framebufferInfo.layers = desc_.layers;

    if (vkCreateFramebuffer(device_.device(), &framebufferInfo, nullptr,
                            &vk_framebuffers_[i]) != VK_SUCCESS) {
      VKR_RES_ERROR("Failed to create framebuffer");
    }
  }
}

void FramebufferSet::destroy() {
  for (auto framebuffer : vk_framebuffers_) {
    if (framebuffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device_.device(), framebuffer, nullptr);
    }
  }

  vk_framebuffers_.clear();
}

void FramebufferSet::update(const FramebufferDesc &desc) {
  destroy();
  desc_ = desc;
  create();
}

} // namespace vkr::resource
