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

auto makeSwapchainRenderPassDesc(const core::Device &device,
                                 const core::Swapchain &swapchain)
    -> RenderPassDesc {
  RenderPassDesc desc{};

  RenderPassColorAttachmentDesc color{};
  color.format = swapchain.format();
  color.samples = VK_SAMPLE_COUNT_1_BIT;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  color.subpassLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  desc.colors.push_back(color);

  desc.depth.enabled = true;
  desc.depth.format = resource::findDepthFormat(device.physicalDevice());
  desc.depth.samples = VK_SAMPLE_COUNT_1_BIT;
  desc.depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  desc.depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  desc.depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  desc.depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  desc.depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  desc.depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  desc.depth.subpassLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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

auto makeOffscreenRenderPassDesc(VkFormat colorFormat, VkFormat depthFormat)
    -> RenderPassDesc {
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

  desc.depth.enabled = true;
  desc.depth.format = depthFormat;
  desc.depth.samples = VK_SAMPLE_COUNT_1_BIT;
  desc.depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  desc.depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  desc.depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  desc.depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  desc.depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  desc.depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  desc.depth.subpassLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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

} // namespace vkr::pipeline
