#include "vkr/resource/targets/offscreen.hh"
#include "vkr/logger.hh"
#include <vulkan/vulkan_core.h>

namespace vkr::resource {

OffscreenTarget::OffscreenTarget(const core::Device &device,
                                 const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool) {
  color_ = std::make_unique<ColorAttachment>(device_, command_pool_);
}

OffscreenTarget::~OffscreenTarget() { destory(); }

void OffscreenTarget::validate() const {
  if (desc_.color.format == VK_FORMAT_UNDEFINED) {
    VKR_RENDER_ERROR("OffscreenTarget color attachment has undefined format");
  }

  if (desc_.color.width == 0 || desc_.color.height == 0) {
    VKR_RENDER_ERROR("OffscreenTarget color attachment has invalid size: {}x{}",
                     desc_.color.width, desc_.color.height);
  }

  if (!desc_.depth) {
    return;
  }

  if (desc_.depth->format == VK_FORMAT_UNDEFINED) {
    VKR_RENDER_ERROR("OffscreenTarget depth attachment has undefined format");
  }

  if (desc_.depth->width == 0 || desc_.depth->height == 0) {
    VKR_RENDER_ERROR("OffscreenTarget depth attachment has invalid size: {}x{}",
                     desc_.depth->width, desc_.depth->height);
  }

  if (desc_.depth->width != desc_.color.width ||
      desc_.depth->height != desc_.color.height) {
    VKR_RENDER_ERROR(
        "OffscreenTarget color/depth size mismatch: color={}x{}, depth={}x{}",
        desc_.color.width, desc_.color.height, desc_.depth->width,
        desc_.depth->height);
  }
}

void OffscreenTarget::create() {
  validate();

  if (!color_) {
    color_ = std::make_unique<ColorAttachment>(device_, command_pool_);
    color_->update(desc_.color);
  }

  color_->create();

  if (desc_.depth) {
    if (!depth_) {
      depth_ = std::make_unique<DepthAttachment>(device_, command_pool_);
      depth_->update(*desc_.depth);
    }

    depth_->create();
  }

  VKR_RENDER_INFO("OffscreenTarget created: {}x{}, depth={}", desc_.color.width,
                  desc_.color.height, depth_ ? "yes" : "no");
}

void OffscreenTarget::destory() {
  depth_.reset();
  color_.reset();
}

void OffscreenTarget::update(const OffscreenTargetDesc &desc) {
  desc_ = desc;
  validate();

  if (!color_) {
    color_ = std::make_unique<ColorAttachment>(device_, command_pool_);
  }

  color_->update(desc_.color);

  if (desc_.depth) {
    if (!depth_) {
      depth_ = std::make_unique<DepthAttachment>(device_, command_pool_);
    }

    depth_->update(*desc_.depth);
  } else {
    depth_.reset();
  }
}

auto OffscreenTarget::attachmentViews() const -> std::vector<VkImageView> {
  std::vector<VkImageView> views{};
  views.reserve(depth_ ? 2 : 1);

  views.push_back(color_->imageView());

  if (depth_) {
    views.push_back(depth_->imageView());
  }

  return views;
}

} // namespace vkr::resource
