#include "vkr/resources/textures/imageview.hh"

namespace vkr::resource {
ImageView::ImageView(const core::Device &device) : device_(device) {}

ImageView::~ImageView() { destroy(); }

void ImageView::create(const Image &image, VkFormat format,
                       VkImageAspectFlags aspectFlags) {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image.image();
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device_.device(), &viewInfo, nullptr, &vk_imageview_) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create image view!");
  }
}
void ImageView::destroy() {
  if (vk_imageview_ != VK_NULL_HANDLE) {
    vkDestroyImageView(device_.device(), vk_imageview_, nullptr);
    vk_imageview_ = VK_NULL_HANDLE;
  }
}

} // namespace vkr::resource
