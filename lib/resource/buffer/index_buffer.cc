#include "vkr/resource/buffer/index_buffer.hh"
#include "vkr/logger.hh"

namespace vkr::resource {
IndexBuffer::IndexBuffer(const core::Device &device,
                         const core::CommandPool &commandPool)
    : device_(device), command_pool_(commandPool),
      target_(std::make_unique<Buffer>(device)) {}

IndexBuffer::~IndexBuffer() = default;

void IndexBuffer::create() {
  if (indices_.empty()) {
    VKR_RES_ERROR("Cannot create index buffer with no indices");
  }

  VkDeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();

  Buffer staging{device_, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
  staging.write(indices_.data(), bufferSize);

  target_->update(bufferSize,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  Buffer::copy(staging, *target_, bufferSize, command_pool_);
}

void IndexBuffer::destroy() {
  target_->destroy();
  indices_.clear();
}

void IndexBuffer::update(const std::vector<uint16_t> &indices) {
  destroy();
  indices_ = indices;
  create();
}

} // namespace vkr::resource
