#include "vkr/resource/image/storage_image.hh"
#include "vkr/logger.hh"
#include "vkr/resource/image/image_commands.hh"

namespace vkr::resource {

StorageImage::StorageImage(const core::Device &device,
                           const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool),
      image_(std::make_unique<Image>(device_)),
      image_view_(std::make_unique<ImageView>(device_)) {}

StorageImage::~StorageImage() { destroy(); }

void StorageImage::update(const StorageImageDesc &desc) {
  desc_ = desc;
  create();
}

void StorageImage::create() {
  destroy();

  if (!desc_.isValid()) {
    VKR_RES_ERROR("StorageImageDesc is invalid");
  }

  ImageDesc imageDesc = ImageDesc::storage2D(desc_.width, desc_.height,
                                             desc_.format);
  imageDesc.usage = desc_.usage;
  imageDesc.aspectMask = desc_.aspectMask;
  imageDesc.defaultViewType = desc_.viewType;
  imageDesc.layout = VK_IMAGE_LAYOUT_UNDEFINED;

  image_->update(imageDesc);

  if (desc_.layout != VK_IMAGE_LAYOUT_UNDEFINED) {
    ImageCommands::transitionLayout(device_, command_pool_, *image_,
                                    VK_IMAGE_LAYOUT_UNDEFINED, desc_.layout);
  }

  image_view_->update(ImageViewDesc::fromImage(*image_));
}

void StorageImage::destroy() {
  if (image_view_) {
    image_view_->destroy();
  }

  if (image_) {
    image_->destroy();
  }
}

auto StorageImage::descriptorInfo() const noexcept -> VkDescriptorImageInfo {
  VkDescriptorImageInfo info{};
  info.sampler = VK_NULL_HANDLE;
  info.imageView = imageView();
  info.imageLayout = layout();
  return info;
}

} // namespace vkr::resource
