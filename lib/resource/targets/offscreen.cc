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

void OffscreenTarget::create() {
  if (!desc_.isValid()) {
    VKR_RES_ERROR("OffscreenTargetDesc params not valid!")
  }

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

  VKR_RES_INFO("OffscreenTarget created: {}x{}, depth={}", desc_.color.width,
               desc_.color.height, depth_ ? "yes" : "no");
}

void OffscreenTarget::destory() {
  depth_.reset();
  color_.reset();
}

void OffscreenTarget::update(const OffscreenTargetDesc &desc) {
  desc_ = desc;
  if (!desc_.isValid()) {
    VKR_RES_ERROR("OffscreenTargetDesc params not valid!")
  }

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
