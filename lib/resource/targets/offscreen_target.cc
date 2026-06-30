#include "vkr/resource/targets/offscreen_target.hh"
#include "vkr/logger.hh"
#include <imgui_impl_vulkan.h>

namespace vkr::resource {

OffscreenTarget::OffscreenTarget(const core::Device &device,
                                 const core::CommandPool &commandPool,
                                 const pipeline::RenderPass &renderPass)
    : device_(device), command_pool_(commandPool), render_pass_(renderPass) {}

OffscreenTarget::~OffscreenTarget() { destroy(); }

void OffscreenTarget::resize(VkExtent2D extent) {
  vkDeviceWaitIdle(device_.device());

  if (imgui_ds_ != VK_NULL_HANDLE) {
    ImGui_ImplVulkan_RemoveTexture(imgui_ds_);
    imgui_ds_ = VK_NULL_HANDLE;
  }

  destroy();
  extent_ = extent;
  create();
}

void OffscreenTarget::create() {
  VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

  auto makeImage = [&](uint32_t w, uint32_t h, VkFormat fmt,
                       VkImageUsageFlags usage, VkImage &img,
                       VkDeviceMemory &mem) -> void {
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

  makeImage(extent_.width, extent_.height, colorFormat,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            color_image_, color_memory_);
  color_view_ = makeView(color_image_, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);

  makeImage(extent_.width, extent_.height, depthFormat,
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

  VKR_RENDER_INFO("OffscreenTarget created ({}x{})", extent_.width,
                  extent_.height);
}

void OffscreenTarget::destroy() {
  auto d = device_.device();
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

auto OffscreenTarget::findMemoryType(uint32_t filter,
                                     VkMemoryPropertyFlags props) -> uint32_t {
  VkPhysicalDeviceMemoryProperties memProps;
  vkGetPhysicalDeviceMemoryProperties(device_.physicalDevice(), &memProps);
  for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
    if ((filter & (1 << i)) &&
        (memProps.memoryTypes[i].propertyFlags & props) == props) {
      return i;
    }
  }
  VKR_RENDER_ERROR("No suitable memory type for offscreen target");
}

} // namespace vkr::resource
