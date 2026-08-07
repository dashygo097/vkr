#include "vkr/exec/render/targets/offscreen.hh"
#include "vkr/logger.hh"
#include <vulkan/vulkan_core.h>

namespace vkr::exec {

OffscreenTarget::OffscreenTarget(const core::Device &device,
                                 const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool) {}

OffscreenTarget::~OffscreenTarget() { destory(); }

void OffscreenTarget::validate() const {
  if (!desc_.colorEnabled && !desc_.depth) {
    VKR_RES_ERROR("OffscreenTarget has no attachments");
  }

  if (desc_.colorEnabled && desc_.color.format == VK_FORMAT_UNDEFINED) {
    VKR_RES_ERROR("OffscreenTarget color attachment has undefined format");
  }

  if (desc_.colorEnabled &&
      (desc_.color.width == 0 || desc_.color.height == 0)) {
    VKR_RES_ERROR("OffscreenTarget color attachment has invalid size: {}x{}",
                  desc_.color.width, desc_.color.height);
  }

  if (!desc_.depth) {
    return;
  }

  if (desc_.depth->format == VK_FORMAT_UNDEFINED) {
    VKR_RES_ERROR("OffscreenTarget depth attachment has undefined format");
  }

  if (desc_.depth->width == 0 || desc_.depth->height == 0) {
    VKR_RES_ERROR("OffscreenTarget depth attachment has invalid size: {}x{}",
                  desc_.depth->width, desc_.depth->height);
  }

  if (desc_.colorEnabled && (desc_.depth->width != desc_.color.width ||
                             desc_.depth->height != desc_.color.height)) {
    VKR_RES_ERROR(
        "OffscreenTarget color/depth size mismatch: color={}x{}, depth={}x{}",
        desc_.color.width, desc_.color.height, desc_.depth->width,
        desc_.depth->height);
  }
}

void OffscreenTarget::create() {
  validate();

  if (desc_.colorEnabled) {
    if (!color_) {
      color_ = std::make_unique<ColorAttachment>(device_, command_pool_);
      color_->update(desc_.color);
    }

    color_->create();
  } else {
    color_.reset();
  }

  if (desc_.depth) {
    if (!depth_) {
      depth_ = std::make_unique<DepthAttachment>(device_, command_pool_);
      depth_->update(*desc_.depth);
    }

    depth_->create();
  } else {
    depth_.reset();
  }

  VKR_RES_INFO("OffscreenTarget created: {}x{}, color={}, depth={}", width(),
               height(), color_ ? "yes" : "no", depth_ ? "yes" : "no");
}

void OffscreenTarget::destory() {
  depth_.reset();
  color_.reset();
}

void OffscreenTarget::update(const OffscreenTargetDesc &desc) {
  desc_ = desc;
  validate();

  if (desc_.colorEnabled) {
    if (!color_) {
      color_ = std::make_unique<ColorAttachment>(device_, command_pool_);
    }

    color_->update(desc_.color);
  } else {
    color_.reset();
  }

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
  views.reserve((color_ ? 1U : 0U) + (depth_ ? 1U : 0U));

  if (color_) {
    views.push_back(color_->imageView());
  }

  if (depth_) {
    views.push_back(depth_->imageView());
  }

  return views;
}

} // namespace vkr::exec
