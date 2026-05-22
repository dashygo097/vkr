#pragma once

#include "../core/device.hh"
#include "../core/swapchain.hh"

namespace vkr::pipeline {

struct RenderPassColorAttachmentDesc {
  VkFormat format{VK_FORMAT_UNDEFINED};
  VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};

  VkAttachmentLoadOp loadOp{VK_ATTACHMENT_LOAD_OP_CLEAR};
  VkAttachmentStoreOp storeOp{VK_ATTACHMENT_STORE_OP_STORE};
  VkAttachmentLoadOp stencilLoadOp{VK_ATTACHMENT_LOAD_OP_DONT_CARE};
  VkAttachmentStoreOp stencilStoreOp{VK_ATTACHMENT_STORE_OP_DONT_CARE};

  VkImageLayout initialLayout{VK_IMAGE_LAYOUT_UNDEFINED};
  VkImageLayout finalLayout{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkImageLayout subpassLayout{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
};

struct RenderPassDepthAttachmentDesc {
  bool enabled{false};

  VkFormat format{VK_FORMAT_UNDEFINED};
  VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};

  VkAttachmentLoadOp loadOp{VK_ATTACHMENT_LOAD_OP_CLEAR};
  VkAttachmentStoreOp storeOp{VK_ATTACHMENT_STORE_OP_DONT_CARE};
  VkAttachmentLoadOp stencilLoadOp{VK_ATTACHMENT_LOAD_OP_DONT_CARE};
  VkAttachmentStoreOp stencilStoreOp{VK_ATTACHMENT_STORE_OP_DONT_CARE};

  VkImageLayout initialLayout{VK_IMAGE_LAYOUT_UNDEFINED};
  VkImageLayout finalLayout{VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  VkImageLayout subpassLayout{VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
};

struct RenderPassDesc {
  std::vector<RenderPassColorAttachmentDesc> colors{};
  RenderPassDepthAttachmentDesc depth{};
  std::vector<VkSubpassDependency> dependencies{};

  [[nodiscard]] auto hasColor() const noexcept -> bool {
    return !colors.empty();
  }

  [[nodiscard]] auto hasDepth() const noexcept -> bool { return depth.enabled; }

  [[nodiscard]] auto attachmentCount() const noexcept -> uint32_t {
    return static_cast<uint32_t>(colors.size() + (depth.enabled ? 1 : 0));
  }
};

class RenderPass {
public:
  explicit RenderPass(const core::Device &device);
  ~RenderPass();

  RenderPass(const RenderPass &) = delete;
  auto operator=(const RenderPass &) -> RenderPass & = delete;

  void create();
  void destroy();
  void update(const RenderPassDesc &desc);

  [[nodiscard]] auto desc() const noexcept -> const RenderPassDesc & {
    return desc_;
  }

  [[nodiscard]] auto renderPass() const noexcept -> VkRenderPass {
    return vk_render_pass_;
  }

private:
  // dependencies
  const core::Device &device_;

  // components
  RenderPassDesc desc_{};
  VkRenderPass vk_render_pass_{VK_NULL_HANDLE};
};

// factories
auto makeSwapchainRenderPassDesc(const core::Device &device,
                                 const core::Swapchain &swapchain)
    -> RenderPassDesc;

} // namespace vkr::pipeline
