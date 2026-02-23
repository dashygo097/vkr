#include "vkr/resources/offscreen_target.hh"
#include "vkr/logger.hh"
#include <imgui_impl_vulkan.h>

namespace vkr::resource {

OffscreenTarget::OffscreenTarget(const core::Device &device,
                                 const core::CommandPool &commandPool,
                                 uint32_t width, uint32_t height)
    : device_(device), command_pool_(commandPool), width_(width),
      height_(height) {
  create();
}

OffscreenTarget::~OffscreenTarget() { destroy(); }

void OffscreenTarget::resize(uint32_t width, uint32_t height) {
  if (imgui_ds_ != VK_NULL_HANDLE) {
    ImGui_ImplVulkan_RemoveTexture(imgui_ds_);
    imgui_ds_ = VK_NULL_HANDLE;
  }
  vkDeviceWaitIdle(device_.device());
  destroy();
  width_ = width;
  height_ = height;
  create();
}

void OffscreenTarget::create() {
  VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

  auto makeImage = [&](uint32_t w, uint32_t h, VkFormat fmt,
                       VkImageUsageFlags usage, VkImage &img,
                       VkDeviceMemory &mem) {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = fmt;
    info.extent = {w, h, 1};
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateImage(device_.device(), &info, nullptr, &img);

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device_.device(), img, &req);
    VkMemoryAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex =
        findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device_.device(), &alloc, nullptr, &mem);
    vkBindImageMemory(device_.device(), img, mem, 0);
  };

  auto makeView = [&](VkImage img, VkFormat fmt,
                      VkImageAspectFlags aspect) -> VkImageView {
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = img;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = fmt;
    info.subresourceRange.aspectMask = aspect;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.layerCount = 1;
    VkImageView view;
    vkCreateImageView(device_.device(), &info, nullptr, &view);
    return view;
  };

  makeImage(width_, height_, colorFormat,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            color_image_, color_memory_);
  color_view_ = makeView(color_image_, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);

  makeImage(width_, height_, depthFormat,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depth_image_,
            depth_memory_);
  depth_view_ = makeView(depth_image_, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

  VkSamplerCreateInfo samp{};
  samp.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samp.magFilter = VK_FILTER_LINEAR;
  samp.minFilter = VK_FILTER_LINEAR;
  samp.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samp.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samp.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  vkCreateSampler(device_.device(), &samp, nullptr, &sampler_);

  VkAttachmentDescription colorAtt{};
  colorAtt.format = colorFormat;
  colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAtt.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentDescription depthAtt{};
  depthAtt.format = depthFormat;
  depthAtt.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAtt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depthRef{
      1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;

  VkSubpassDependency dep{};
  dep.srcSubpass = 0;
  dep.dstSubpass = VK_SUBPASS_EXTERNAL;
  dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  std::array<VkAttachmentDescription, 2> atts = {colorAtt, depthAtt};
  VkRenderPassCreateInfo rpInfo{};
  rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rpInfo.attachmentCount = static_cast<uint32_t>(atts.size());
  rpInfo.pAttachments = atts.data();
  rpInfo.subpassCount = 1;
  rpInfo.pSubpasses = &subpass;
  rpInfo.dependencyCount = 1;
  rpInfo.pDependencies = &dep;
  vkCreateRenderPass(device_.device(), &rpInfo, nullptr, &render_pass_);

  std::array<VkImageView, 2> views = {color_view_, depth_view_};
  VkFramebufferCreateInfo fbInfo{};
  fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fbInfo.renderPass = render_pass_;
  fbInfo.attachmentCount = static_cast<uint32_t>(views.size());
  fbInfo.pAttachments = views.data();
  fbInfo.width = width_;
  fbInfo.height = height_;
  fbInfo.layers = 1;
  vkCreateFramebuffer(device_.device(), &fbInfo, nullptr, &framebuffer_);

  VKR_RENDER_INFO("OffscreenTarget created ({}x{})", width_, height_);
}

void OffscreenTarget::destroy() {
  auto d = device_.device();
  if (framebuffer_) {
    vkDestroyFramebuffer(d, framebuffer_, nullptr);
    framebuffer_ = VK_NULL_HANDLE;
  }
  if (render_pass_) {
    vkDestroyRenderPass(d, render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;
  }
  if (sampler_) {
    vkDestroySampler(d, sampler_, nullptr);
    sampler_ = VK_NULL_HANDLE;
  }
  if (color_view_) {
    vkDestroyImageView(d, color_view_, nullptr);
    color_view_ = VK_NULL_HANDLE;
  }
  if (color_image_) {
    vkDestroyImage(d, color_image_, nullptr);
    color_image_ = VK_NULL_HANDLE;
  }
  if (color_memory_) {
    vkFreeMemory(d, color_memory_, nullptr);
    color_memory_ = VK_NULL_HANDLE;
  }
  if (depth_view_) {
    vkDestroyImageView(d, depth_view_, nullptr);
    depth_view_ = VK_NULL_HANDLE;
  }
  if (depth_image_) {
    vkDestroyImage(d, depth_image_, nullptr);
    depth_image_ = VK_NULL_HANDLE;
  }
  if (depth_memory_) {
    vkFreeMemory(d, depth_memory_, nullptr);
    depth_memory_ = VK_NULL_HANDLE;
  }
}

void OffscreenTarget::registerWithImGui(VkDescriptorPool descriptorPool) {
  imgui_ds_ = ImGui_ImplVulkan_AddTexture(
      sampler_, color_view_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  VKR_RENDER_INFO("OffscreenTarget registered with ImGui.");
}

uint32_t OffscreenTarget::findMemoryType(uint32_t filter,
                                         VkMemoryPropertyFlags props) {
  VkPhysicalDeviceMemoryProperties memProps;
  vkGetPhysicalDeviceMemoryProperties(device_.physicalDevice(), &memProps);
  for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
    if ((filter & (1 << i)) &&
        (memProps.memoryTypes[i].propertyFlags & props) == props)
      return i;
  VKR_RENDER_ERROR("No suitable memory type for offscreen target");
}

} // namespace vkr::resource
