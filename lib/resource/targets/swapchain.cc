#include "vkr/resource/targets/swapchain.hh"
#include "vkr/logger.hh"

namespace vkr::resource {

SwapchainTarget::SwapchainTarget(const core::Device &device,
                                 const core::CommandPool &commandPool,
                                 const core::Swapchain &swapchain)
    : device_(device), command_pool_(commandPool), swapchain_(swapchain) {}

SwapchainTarget::~SwapchainTarget() { destory(); }

void SwapchainTarget::create() {
  destory();

  if (swapchain_.imageCount() == 0) {
    VKR_RES_ERROR("SwapchainTarget has no swapchain images");
  }

  if (!desc_.depth) {
    return;
  }

  if (desc_.depth->format == VK_FORMAT_UNDEFINED) {
    VKR_RES_ERROR("SwapchainTarget depth attachment has undefined format");
  }

  DepthAttachmentDesc depthDesc = *desc_.depth;
  depthDesc.width = swapchain_.extent2D().width;
  depthDesc.height = swapchain_.extent2D().height;

  depth_ = std::make_unique<DepthAttachment>(device_, command_pool_);
  depth_->update(depthDesc);

  desc_.depth = depthDesc;
}

void SwapchainTarget::destory() {
  if (depth_) {
    depth_->destory();
    depth_.reset();
  }
}

void SwapchainTarget::update(const SwapchainTargetDesc &desc) {
  desc_ = desc;
  create();
}

auto SwapchainTarget::attachmentViews(size_t imageIndex) const
    -> std::vector<VkImageView> {
  std::vector<VkImageView> views{};
  views.reserve(depth_ ? 2 : 1);

  views.push_back(swapchain_.imageView(imageIndex));

  if (depth_) {
    views.push_back(depth_->imageView());
  }

  return views;
}

auto SwapchainTarget::attachmentViews() const
    -> std::vector<std::vector<VkImageView>> {
  std::vector<std::vector<VkImageView>> views{};
  views.reserve(swapchain_.imageCount());

  for (size_t i = 0; i < swapchain_.imageCount(); i++) {
    views.push_back(attachmentViews(i));
  }

  return views;
}

} // namespace vkr::resource
