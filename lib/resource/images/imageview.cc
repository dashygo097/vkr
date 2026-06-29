#include "vkr/resource/images/imageview.hh"
#include "vkr/logger.hh"

namespace vkr::resource {

ImageView::ImageView(const core::Device &device) : device_(device) {}

ImageView::~ImageView() { destroy(); }

void ImageView::create() {
  destroy();

  if (desc_.image == VK_NULL_HANDLE) {
    VKR_RES_ERROR("ImageViewDesc has null image");
  }

  if (desc_.format == VK_FORMAT_UNDEFINED) {
    VKR_RES_ERROR("ImageViewDesc has undefined format");
  }

  if (desc_.aspectMask == 0) {
    VKR_RES_ERROR("ImageViewDesc has no aspect mask");
  }

  if (desc_.levelCount == 0) {
    VKR_RES_ERROR("ImageViewDesc has zero mip level count");
  }

  if (desc_.layerCount == 0) {
    VKR_RES_ERROR("ImageViewDesc has zero array layer count");
  }

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = desc_.image;
  viewInfo.viewType = desc_.viewType;
  viewInfo.format = desc_.format;
  viewInfo.components = desc_.components;
  viewInfo.subresourceRange.aspectMask = desc_.aspectMask;
  viewInfo.subresourceRange.baseMipLevel = desc_.baseMipLevel;
  viewInfo.subresourceRange.levelCount = desc_.levelCount;
  viewInfo.subresourceRange.baseArrayLayer = desc_.baseArrayLayer;
  viewInfo.subresourceRange.layerCount = desc_.layerCount;

  if (vkCreateImageView(device_.device(), &viewInfo, nullptr, &vk_imageview_) !=
      VK_SUCCESS) {
    VKR_RES_ERROR("Failed to create image view");
  }
}

void ImageView::destroy() {
  if (vk_imageview_ != VK_NULL_HANDLE) {
    vkDestroyImageView(device_.device(), vk_imageview_, nullptr);
    vk_imageview_ = VK_NULL_HANDLE;
  }
}

void ImageView::update(const ImageViewDesc &desc) {
  destroy();
  desc_ = desc;
  create();
}

} // namespace vkr::resource
