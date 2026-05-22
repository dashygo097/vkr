#include "vkr/pipeline/render_pass.hh"
#include "vkr/logger.hh"
#include "vkr/resources/depth_resources.hh"

namespace vkr::pipeline {

RenderPass::RenderPass(const core::Device &device) : device_(device) {}

RenderPass::~RenderPass() { destroy(); }

void RenderPass::create() {
  destroy();

  if (desc_.attachmentCount() == 0) {
    VKR_PIPE_ERROR("RenderPassDesc has no attachments");
  }

  std::vector<VkAttachmentDescription> attachments{};
  std::vector<VkAttachmentReference> colorRefs{};

  attachments.reserve(desc_.attachmentCount());
  colorRefs.reserve(desc_.colors.size());

  for (uint32_t i = 0; i < desc_.colors.size(); i++) {
    const auto &color = desc_.colors[i];

    if (color.format == VK_FORMAT_UNDEFINED) {
      VKR_PIPE_ERROR("RenderPass color attachment {} has undefined format", i);
    }

    VkAttachmentDescription attachment{};
    attachment.format = color.format;
    attachment.samples = color.samples;
    attachment.loadOp = color.loadOp;
    attachment.storeOp = color.storeOp;
    attachment.stencilLoadOp = color.stencilLoadOp;
    attachment.stencilStoreOp = color.stencilStoreOp;
    attachment.initialLayout = color.initialLayout;
    attachment.finalLayout = color.finalLayout;
    attachments.push_back(attachment);

    VkAttachmentReference ref{};
    ref.attachment = i;
    ref.layout = color.subpassLayout;
    colorRefs.push_back(ref);
  }

  VkAttachmentReference depthRef{};
  if (desc_.depth.enabled) {
    if (desc_.depth.format == VK_FORMAT_UNDEFINED) {
      VKR_PIPE_ERROR("RenderPass depth attachment has undefined format");
    }

    VkAttachmentDescription attachment{};
    attachment.format = desc_.depth.format;
    attachment.samples = desc_.depth.samples;
    attachment.loadOp = desc_.depth.loadOp;
    attachment.storeOp = desc_.depth.storeOp;
    attachment.stencilLoadOp = desc_.depth.stencilLoadOp;
    attachment.stencilStoreOp = desc_.depth.stencilStoreOp;
    attachment.initialLayout = desc_.depth.initialLayout;
    attachment.finalLayout = desc_.depth.finalLayout;
    attachments.push_back(attachment);

    depthRef.attachment = static_cast<uint32_t>(attachments.size() - 1);
    depthRef.layout = desc_.depth.subpassLayout;
  }

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
  subpass.pColorAttachments = colorRefs.empty() ? nullptr : colorRefs.data();
  subpass.pDepthStencilAttachment = desc_.depth.enabled ? &depthRef : nullptr;

  auto dependencies = desc_.dependencies;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies =
      dependencies.empty() ? nullptr : dependencies.data();

  if (vkCreateRenderPass(device_.device(), &renderPassInfo, nullptr,
                         &vk_render_pass_) != VK_SUCCESS) {
    VKR_PIPE_ERROR("Failed to create render pass");
  }

  VKR_PIPE_INFO("Render pass created successfully");
}

void RenderPass::destroy() {
  if (vk_render_pass_ != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device_.device(), vk_render_pass_, nullptr);
    vk_render_pass_ = VK_NULL_HANDLE;
  }
}

void RenderPass::update(const RenderPassDesc &desc) {
  destroy();
  desc_ = desc;
  create();
}

} // namespace vkr::pipeline
