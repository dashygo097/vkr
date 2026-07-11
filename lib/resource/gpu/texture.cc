#include "vkr/resource/gpu/texture.hh"
#include "vkr/logger.hh"

namespace vkr::resource {

Texture::Texture(const core::Device &device,
                 const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool),
      image_(std::make_unique<Image>(device_, command_pool_)),
      image_view_(std::make_unique<ImageView>(device_)),
      sampler_(std::make_unique<Sampler>(device_)) {}

Texture::~Texture() { destroy(); }

void Texture::update(const TextureDesc &desc) {
  desc_ = desc;
  create();
}

void Texture::create() {
  destroy();

  if (!desc_.isValid()) {
    VKR_RES_ERROR("TextureDesc is invalid");
  }

  image_->update(desc_.image);
  image_view_->update(desc_.useDefaultView ? ImageViewDesc::fromImage(*image_)
                                           : desc_.view);
  if (desc_.createSampler) {
    sampler_->update(desc_.sampler);
  } else {
    sampler_->destroy();
  }
}

void Texture::destroy() {
  if (sampler_) {
    sampler_->destroy();
  }

  if (image_view_) {
    image_view_->destroy();
  }

  if (image_) {
    image_->destroy();
  }
}

} // namespace vkr::resource
