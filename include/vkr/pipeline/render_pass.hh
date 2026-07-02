#pragma once

#include "vkr/core/device.hh"

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

  // factories
  [[nodiscard]] static auto
  makeSwapchain(VkFormat colorFormat,
                VkFormat depthFormat = VK_FORMAT_UNDEFINED) -> RenderPassDesc {
    RenderPassDesc desc{};

    RenderPassColorAttachmentDesc color{};
    color.format = colorFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color.subpassLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    desc.colors.push_back(color);

    if (depthFormat != VK_FORMAT_UNDEFINED) {
      desc.depth.enabled = true;
      desc.depth.format = depthFormat;
      desc.depth.samples = VK_SAMPLE_COUNT_1_BIT;
      desc.depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      desc.depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      desc.depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      desc.depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      desc.depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      desc.depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      desc.depth.subpassLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    } else {
      desc.depth.enabled = false;
    }

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    desc.dependencies.push_back(dependency);

    return desc;
  }

  [[nodiscard]] static auto
  makeOffscreen(VkFormat colorFormat,
                VkFormat depthFormat = VK_FORMAT_UNDEFINED) -> RenderPassDesc {
    RenderPassDesc desc{};

    RenderPassColorAttachmentDesc color{};
    color.format = colorFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    color.subpassLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    desc.colors.push_back(color);

    if (depthFormat != VK_FORMAT_UNDEFINED) {
      desc.depth.enabled = true;
      desc.depth.format = depthFormat;
      desc.depth.samples = VK_SAMPLE_COUNT_1_BIT;
      desc.depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      desc.depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      desc.depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      desc.depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      desc.depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      desc.depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      desc.depth.subpassLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    } else {
      desc.depth.enabled = false;
    }

    VkSubpassDependency before{};
    before.srcSubpass = VK_SUBPASS_EXTERNAL;
    before.dstSubpass = 0;
    before.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    before.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    before.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    before.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    desc.dependencies.push_back(before);

    VkSubpassDependency after{};
    after.srcSubpass = 0;
    after.dstSubpass = VK_SUBPASS_EXTERNAL;
    after.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    after.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    after.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    after.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    desc.dependencies.push_back(after);

    return desc;
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

} // namespace vkr::pipeline
