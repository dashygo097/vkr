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

  if (swapchain_.swapchain() == VK_NULL_HANDLE) {
    VKR_RES_ERROR("SwapchainTarget has null swapchain");
  }

  uint32_t imageCount = 0;
  vkGetSwapchainImagesKHR(device_.device(), swapchain_.swapchain(), &imageCount,
                          nullptr);

  if (imageCount == 0) {
    VKR_RES_ERROR("SwapchainTarget has no swapchain images");
  }

  vk_color_images_.resize(imageCount);

  vkGetSwapchainImagesKHR(device_.device(), swapchain_.swapchain(), &imageCount,
                          vk_color_images_.data());

  color_image_views_.reserve(vk_color_images_.size());

  for (auto image : vk_color_images_) {
    auto imageView = std::make_unique<ImageView>(device_);
    imageView->update(ImageViewDesc::color2D(image, swapchain_.format()));
    color_image_views_.push_back(std::move(imageView));
  }

  if (desc_.depth) {
    DepthAttachmentDesc depthDesc = *desc_.depth;
    depthDesc.width = swapchain_.extent2D().width;
    depthDesc.height = swapchain_.extent2D().height;

    if (depthDesc.format == VK_FORMAT_UNDEFINED) {
      VKR_RES_ERROR("SwapchainTarget depth attachment has undefined format");
    }

    depth_ = std::make_unique<DepthAttachment>(device_, command_pool_);
    depth_->update(depthDesc);
    desc_.depth = depthDesc;
  }

  VKR_RES_INFO("SwapchainTarget created: extent={}x{}, images={}, depth={}",
               swapchain_.extent2D().width, swapchain_.extent2D().height,
               vk_color_images_.size(), depth_ ? "yes" : "no");
}

void SwapchainTarget::destory() {
  if (depth_) {
    depth_->destory();
    depth_.reset();
  }

  for (auto &imageView : color_image_views_) {
    if (imageView) {
      imageView->destroy();
    }
  }

  color_image_views_.clear();
  vk_color_images_.clear();
}

void SwapchainTarget::update(const SwapchainTargetDesc &desc) {
  desc_ = desc;
  create();
}

auto SwapchainTarget::imageViews() const -> std::vector<VkImageView> {
  std::vector<VkImageView> views{};
  views.reserve(color_image_views_.size());

  for (const auto &imageView : color_image_views_) {
    views.push_back(imageView->imageView());
  }

  return views;
}

auto SwapchainTarget::attachmentViews(size_t imageIndex) const
    -> std::vector<VkImageView> {
  std::vector<VkImageView> views{};
  views.reserve(depth_ ? 2 : 1);

  views.push_back(imageView(imageIndex));

  if (depth_) {
    views.push_back(depth_->imageView());
  }

  return views;
}

auto SwapchainTarget::attachmentViews() const
    -> std::vector<std::vector<VkImageView>> {
  std::vector<std::vector<VkImageView>> views{};
  views.reserve(vk_color_images_.size());

  for (size_t i = 0; i < vk_color_images_.size(); i++) {
    views.push_back(attachmentViews(i));
  }

  return views;
}

} // namespace vkr::resource
