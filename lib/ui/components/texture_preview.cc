#include "texture_preview.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <glm/gtc/packing.hpp>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>
#include <string_view>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_format_traits.hpp>

namespace vkr::ui {
namespace {

// Borrows the preview's selected view and sampler; owns no GPU resources.
struct PreviewTexture {
  const resource::ImageView &view;
  const resource::Sampler &sampler;
  VkImageLayout layout;

  [[nodiscard]] auto descriptorInfo() const noexcept -> VkDescriptorImageInfo {
    return {sampler.sampler(), view.imageView(), layout};
  }
};

constexpr auto VertexShader = R"glsl(
#version 450
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 0) out vec2 texCoord;
layout(push_constant) uniform Projection { vec2 scale; vec2 translate; } projection;
void main() {
  texCoord = uv;
  gl_Position = vec4(position * projection.scale + projection.translate, 0, 1);
}
)glsl";

constexpr auto FragmentShader = R"glsl(
#version 450
layout(set = 0, binding = 0) uniform sampler2D image;
layout(set = 1, binding = 0, std140) uniform Display {
  int channel;
  int depth;
  int toneMap;
  int encodeSrgb;
  float exposure;
  float depthMin;
  float depthMax;
  float padding;
} display;
layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

vec3 encode(vec3 v) {
  return mix(1.055 * pow(v, vec3(1.0 / 2.4)) - 0.055,
             12.92 * v, lessThanEqual(v, vec3(0.0031308)));
}
vec3 decode(vec3 v) {
  return mix(pow((v + 0.055) / 1.055, vec3(2.4)),
             v / 12.92, lessThanEqual(v, vec3(0.04045)));
}
void main() {
  vec4 value = textureLod(image, texCoord, 0);
  if (any(isnan(value)) || any(isinf(value))) {
    outColor = vec4(1, 0, 1, 1);
    return;
  }
  if (display.depth != 0) {
    value = vec4(vec3(clamp((value.r - display.depthMin) /
                       max(display.depthMax - display.depthMin, 1e-7), 0, 1)), 1);
  } else if (display.channel == 5) {
    value = vec4(vec3(clamp(value.a, 0, 1)), 1);
  } else {
    value.rgb = max(value.rgb * exp2(display.exposure), vec3(0));
    if (display.toneMap != 0)
      value.rgb = value.rgb / (vec3(1) + value.rgb);
    if (display.channel >= 2)
      value = vec4(vec3(value[display.channel - 2]), 1);
    else if (display.channel == 1)
      value.a = 1;
  }
  if (display.encodeSrgb > 0)
    value.rgb = encode(max(value.rgb, vec3(0)));
  else if (display.encodeSrgb < 0)
    value.rgb = decode(clamp(value.rgb, 0, 1));
  outColor = vec4(value.rgb, clamp(value.a, 0, 1));
}
)glsl";

// Readback is deliberately limited to unpacked formats. Compressed/packed
// images are still viewable, but are not misinterpreted as RGBA byte arrays.
auto texelFormat(VkFormat format) -> bool {
  const auto value = static_cast<vk::Format>(format);
  if (vk::isCompressed(value) || vk::planeCount(value) != 1 ||
      vk::blockSize(value) > 16 || vk::componentCount(value) == 0) {
    return false;
  }
  uint32_t bytes{0};
  for (uint8_t component = 0; component < vk::componentCount(value);
       ++component) {
    const auto bits = vk::componentBits(value, component);
    const std::string_view type = vk::componentNumericFormat(value, component);
    if (!((bits == 8 || bits == 16) &&
          (type == "UNORM" || type == "SRGB" || type == "SNORM")) &&
        !(type == "SFLOAT" && (bits == 16 || bits == 32))) {
      return false;
    }
    bytes += bits / 8;
  }
  return bytes == vk::blockSize(value);
}

auto readTexel(VkFormat format, const resource::Buffer &buffer)
    -> std::array<float, 4> {
  const auto value = static_cast<vk::Format>(format);
  std::array<float, 4> result{0.0f, 0.0f, 0.0f, 1.0f};
  size_t offset{0};
  for (uint8_t component = 0; component < vk::componentCount(value);
       ++component) {
    const auto bits = vk::componentBits(value, component);
    const std::string_view type = vk::componentNumericFormat(value, component);
    const std::string_view name = vk::componentName(value, component);
    uint32_t data{0};
    std::memcpy(&data, static_cast<const std::byte *>(buffer.mapped()) + offset,
                bits / 8);
    offset += bits / 8;
    float number{0.0f};
    if (type == "SFLOAT") {
      if (bits == 16) {
        number = glm::unpackHalf1x16(static_cast<uint16_t>(data));
      } else {
        std::memcpy(&number, &data, sizeof(number));
      }
    } else if (type == "SNORM") {
      const auto signedValue =
          bits == 8 ? static_cast<int8_t>(data) : static_cast<int16_t>(data);
      number = std::max(-1.0f, static_cast<float>(signedValue) /
                                   (bits == 8 ? 127.0f : 32767.0f));
    } else {
      number = static_cast<float>(data) / (bits == 8 ? 255.0f : 65535.0f);
    }
    if (name == "D") {
      result[0] = result[1] = result[2] = number;
    } else {
      const size_t index = name == "R"   ? 0
                           : name == "G" ? 1
                           : name == "B" ? 2
                                         : 3;
      result[index] = number;
    }
  }
  return result;
}

} // namespace

TexturePreview::Frame::Frame(const core::Device &device)
    : views{resource::ImageView{device}, resource::ImageView{device}},
      parameters{resource::UniformBuffer<Parameters>{device},
                 resource::UniformBuffer<Parameters>{device}},
      readback(device) {}

TexturePreview::TexturePreview(const core::Device &device,
                               const pipeline::RenderPass &renderPass,
                               const core::CommandBuffers &commandBuffers,
                               const scene::Scene &scene,
                               const Selection &selection)
    : device_(device), render_pass_(renderPass),
      command_buffers_(commandBuffers), scene_(scene), selection_(selection),
      image_layout_(device), parameter_layout_(device),
      descriptor_pool_(device),
      nearest_sampler_(device), linear_sampler_(device),
      pipeline_(device, renderPass) {
  static_assert(sizeof(Parameters) == 32);
  static_assert(offsetof(Parameters, exposure) == 16);
  image_layout_.update(
      {.bindings = {
           {.layout = {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            }}}});
  parameter_layout_.update(
      {.bindings = {{.layout = {
                         .binding = 0,
                         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                         .descriptorCount = 1,
                         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                     }}}});
  descriptor_pool_.update({
      .poolSizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                     commandBuffers.size() * 2U},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                     commandBuffers.size() * 2U}},
      .maxSets = commandBuffers.size() * 4U,
  });
  image_sets_.reserve(commandBuffers.size() * 2U);
  parameter_sets_.reserve(commandBuffers.size() * 2U);
  nearest_sampler_.update(resource::SamplerDesc::nearestClampToEdge());
  linear_sampler_.update(resource::SamplerDesc::linearClampToEdge());
  draws_.reserve(commandBuffers.size() * 2U);
  for (uint32_t frame = 0; frame < commandBuffers.size(); ++frame) {
    frames_.push_back(std::make_unique<Frame>(device));
    for (uint32_t slot = 0; slot < 2; ++slot) {
      image_sets_.emplace_back(device_, descriptor_pool_, image_layout_);
      image_sets_.back().create();
      parameter_sets_.emplace_back(device_, descriptor_pool_, parameter_layout_);
      auto &parameters = parameter_sets_.back();
      parameters.uniform(0, frames_.back()->parameters[slot]);
      parameters.create();
      draws_.push_back({pipeline_, image_sets_.back(), parameters});
    }
  }
}

TexturePreview::~TexturePreview() {
  // Pipeline destruction waits for outstanding draws before views/buffers die.
  pipeline_.destroy();
}

auto TexturePreview::image() const -> std::optional<Image> {
  Image result{};
  if (selection_.type == SelectionType::Texture) {
    const auto texture = scene_.findTexture(selection_.name);
    if (!texture || !texture->get().valid()) {
      return std::nullopt;
    }
    const auto &value = texture->get();
    const auto &desc = value.desc();
    if (!desc.useDefaultView && desc.view.image != value.image()) {
      return std::nullopt;
    }
    result.image = desc.image;
    result.image.width = value.width();
    result.image.height = value.height();
    result.layout = value.layout();
    result.view = desc.view;
    if (desc.useDefaultView) {
      result.view = {};
      result.view.image = value.image();
      result.view.format = desc.image.format;
      result.view.viewType = desc.image.defaultViewType;
      result.view.aspectMask = desc.image.aspectMask;
      result.view.levelCount = desc.image.mipLevels;
      result.view.layerCount = desc.image.arrayLayers;
    }
    result.cubemap = result.view.viewType == VK_IMAGE_VIEW_TYPE_CUBE ||
                     result.view.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
  } else if (selection_.type == SelectionType::Cubemap) {
    const auto cubemap = scene_.findCubemap(selection_.name);
    if (!cubemap || !cubemap->get().valid()) {
      return std::nullopt;
    }
    const auto &value = cubemap->get();
    result.image = resource::ImageDesc::sampled2D(value.width(), value.height(),
                                                  value.desc().format);
    result.image.arrayLayers = 6;
    result.view =
        resource::ImageViewDesc::cubemap(value.image(), value.desc().format);
    result.layout = value.layout();
    result.cubemap = true;
  } else {
    return std::nullopt;
  }
  if (result.view.baseMipLevel >= result.image.mipLevels ||
      result.view.baseArrayLayer >= result.image.arrayLayers) {
    return std::nullopt;
  }
  if (result.view.levelCount == VK_REMAINING_MIP_LEVELS) {
    result.view.levelCount = result.image.mipLevels - result.view.baseMipLevel;
  }
  if (result.view.layerCount == VK_REMAINING_ARRAY_LAYERS) {
    result.view.layerCount =
        result.image.arrayLayers - result.view.baseArrayLayer;
  }
  VkFormatProperties properties{};
  vkGetPhysicalDeviceFormatProperties(device_.physicalDevice(),
                                      result.view.format, &properties);
  result.features = result.image.tiling == VK_IMAGE_TILING_OPTIMAL
                        ? properties.optimalTilingFeatures
                        : properties.linearTilingFeatures;
  return result;
}

auto TexturePreview::originalView() const -> VkImageView {
  if (selection_.type == SelectionType::Texture) {
    const auto texture = scene_.findTexture(selection_.name);
    return texture ? texture->get().imageView() : VK_NULL_HANDLE;
  }
  const auto cubemap = scene_.findCubemap(selection_.name);
  return cubemap ? cubemap->get().imageView() : VK_NULL_HANDLE;
}

auto TexturePreview::validate(const Image &value) const -> std::string_view {
  if (value.image.type != VK_IMAGE_TYPE_2D ||
      value.image.samples != VK_SAMPLE_COUNT_1_BIT || value.image.width == 0 ||
      value.image.height == 0 ||
      (value.image.usage & VK_IMAGE_USAGE_SAMPLED_BIT) == 0) {
    return "Preview requires a sampled, single-sample 2D image.";
  }
  if (value.view.viewType != VK_IMAGE_VIEW_TYPE_2D &&
      value.view.viewType != VK_IMAGE_VIEW_TYPE_2D_ARRAY &&
      value.view.viewType != VK_IMAGE_VIEW_TYPE_CUBE &&
      value.view.viewType != VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
    return "This image view type is not supported.";
  }
  if (value.view.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT &&
      value.view.aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT) {
    return "Preview supports color or depth views, not stencil/combined views.";
  }
  if (value.view.levelCount == 0 || value.view.layerCount == 0 ||
      value.view.levelCount > value.image.mipLevels - value.view.baseMipLevel ||
      value.view.layerCount >
          value.image.arrayLayers - value.view.baseArrayLayer) {
    return "The image view has an invalid subresource range.";
  }
  if ((value.image.usage &
       (VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) != 0 ||
      (value.layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
       value.layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)) {
    return "Preview requires a read-only image. Live render/storage "
           "attachments "
           "need graph-managed synchronization and are not sampled here.";
  }
  const auto format = static_cast<vk::Format>(value.view.format);
  if (value.view.format != value.image.format) {
    return "Reinterpreted image formats require a specialized preview.";
  }
  if (vk::planeCount(format) != 1 || vk::componentCount(format) == 0) {
    return "Multi-plane images require a specialized preview.";
  }
  const auto extent = vk::blockExtent(format);
  if (!vk::isCompressed(format) && (extent[0] != 1 || extent[1] != 1)) {
    return "Packed chroma formats require a specialized preview.";
  }
  const std::string_view type = vk::componentNumericFormat(format, 0);
  if (type != "UNORM" && type != "SRGB" && type != "SNORM" &&
      type != "SFLOAT" && type != "UFLOAT") {
    return "Integer images require an integer-sampler preview.";
  }
  if ((value.features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == 0) {
    return "This device cannot sample the image format.";
  }
  return {};
}

void TexturePreview::createPipeline() {
  scene::VertexInputDesc input{};
  input.bindings = {{0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX}};
  input.attributes = {
      {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)},
      {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)},
  };
  auto desc =
      pipeline::GraphicsPipelineDesc::fullscreen("Inspector preview")
          .vertexInputDesc(std::move(input))
          .vertexShader(resource::ShaderModuleDesc::vertexGlslSource(
              VertexShader, "inspector_preview.vert"))
          .fragmentShader(resource::ShaderModuleDesc::fragmentGlslSource(
              FragmentShader, "inspector_preview.frag"))
          .descriptorSetLayout(image_layout_.layout())
          .descriptorSetLayout(parameter_layout_.layout())
          // Keep set 0 and push constants compatible with ImGui.
          .pushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4)
          .alphaBlend();
  desc.colorBlend.attachments[0].dstAlphaBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  if (!pipeline_.update(desc)) {
    VKR_UI_ERROR("Failed to create Inspector preview pipeline");
  }
}

void TexturePreview::prepare(uint32_t frameIndex) {
  frame_index_ = frameIndex;
  auto &frame = *frames_.at(frameIndex);
  // The application has already waited for this slot's fence. Coherent mapped
  // memory is read only here, never by waiting for a probe in the UI thread.
  if (frame.probe) {
    const auto current = image();
    const auto &probe = *frame.probe;
    if (current && selection_.type == probe.selection.type &&
        selection_.name == probe.selection.name &&
        current->view.image == probe.view.image &&
        originalView() == probe.originalView) {
      completed_probe_ = probe;
      texel_ = readTexel(probe.view.format, frame.readback);
    }
    frame.probe.reset();
  }
  if (!requested_probe_) {
    return;
  }
  auto probe = std::move(*requested_probe_);
  requested_probe_.reset();
  const auto current = image();
  if (!current || !validate(*current).empty() ||
      selection_.type != probe.selection.type ||
      selection_.name != probe.selection.name ||
      current->view.image != probe.view.image ||
      originalView() != probe.originalView ||
      (current->image.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0 ||
      !texelFormat(probe.view.format)) {
    return;
  }
  if (!frame.readback.isValid()) {
    frame.readback.update(16, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    (void)frame.readback.map();
  }
  const auto commandBuffer = command_buffers_.buffer(frameIndex);
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.oldLayout = current->layout;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = probe.view.image;
  barrier.subresourceRange = {probe.view.aspectMask, probe.view.baseMipLevel, 1,
                              probe.view.baseArrayLayer, 1};
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);
  VkBufferImageCopy region{};
  region.imageSubresource = {probe.view.aspectMask, probe.view.baseMipLevel,
                             probe.view.baseArrayLayer, 1};
  region.imageOffset = {static_cast<int32_t>(probe.x),
                        static_cast<int32_t>(probe.y), 0};
  region.imageExtent = {1, 1, 1};
  vkCmdCopyImageToBuffer(commandBuffer, probe.view.image,
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         frame.readback.buffer(), 1, &region);
  std::swap(barrier.oldLayout, barrier.newLayout);
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  VkBufferMemoryBarrier hostBarrier{};
  hostBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  hostBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  hostBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  hostBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  hostBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  hostBarrier.buffer = frame.readback.buffer();
  hostBarrier.size = VK_WHOLE_SIZE;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT |
                           VK_PIPELINE_STAGE_HOST_BIT,
                       0, 0, nullptr, 1, &hostBarrier, 1, &barrier);
  frame.probe = std::move(probe);
}

void TexturePreview::controls(const Image &value, bool enlarged) {
  const float width = ImGui::GetFontSize() * 7.0f;
  if (value.view.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT) {
    ImGui::SetNextItemWidth(width);
    ImGui::Combo("Channel", &channel_, "RGBA\0RGB\0R\0G\0B\0A\0");
  }
  if (value.view.levelCount > 1) {
    ImGui::SetNextItemWidth(width);
    if (ImGui::SliderInt("Mip", &mip_, 0,
                         static_cast<int>(value.view.levelCount - 1))) {
      fit_ = true;
      pan_ = {};
    }
  }
  ImGui::SetNextItemWidth(width);
  if (value.cubemap && value.view.layerCount == 6) {
    if (ImGui::Combo("Face", &layer_, "+X\0-X\0+Y\0-Y\0+Z\0-Z\0")) {
      pan_ = {};
    }
  } else if (value.view.layerCount > 1) {
    ImGui::SliderInt("Layer", &layer_, 0,
                     static_cast<int>(value.view.layerCount - 1));
  }
  if (value.view.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT) {
    ImGui::SetNextItemWidth(width * 2);
    ImGui::DragFloatRange2("Depth range", &depth_min_, &depth_max_, 0.001f,
                           0.0f, 1.0f, "%.4f", "%.4f");
  } else {
    ImGui::SetNextItemWidth(width * 2);
    ImGui::SliderFloat("Exposure (EV)", &exposure_, -16.0f, 16.0f, "%.2f");
    ImGui::Checkbox("Tone map", &tone_map_);
    ImGui::SameLine();
    ImGui::Checkbox("Linear color", &srgb_);
  }
  const bool canFilter =
      (value.features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
  ImGui::BeginDisabled(!canFilter);
  ImGui::Checkbox("Linear filter", &linear_);
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::Checkbox("Flip Y", &flip_y_);
  if (enlarged) {
    if (ImGui::Button("Fit")) {
      fit_ = true;
      pan_ = {};
    }
    ImGui::SameLine();
    if (ImGui::Button("1:1")) {
      fit_ = false;
      zoom_ = 1.0f / std::max(0.01f, ImGui::GetIO().DisplayFramebufferScale.x);
      pan_ = {};
    }
    ImGui::TextDisabled("Wheel: zoom | Middle drag: pan | Click: sample");
  }
}

void TexturePreview::bind(const ImDrawList *, const ImDrawCmd *command) {
  const auto &draw = *static_cast<const Draw *>(command->UserCallbackData);
  const auto &state = *static_cast<const ImGui_ImplVulkan_RenderState *>(
      ImGui::GetPlatformIO().Renderer_RenderState);
  vkCmdBindPipeline(state.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    draw.pipeline.get().pipeline());
  // Bind both sets with our layout first. ImGui may then rebind set 0 with its
  // compatible layout without disturbing the display parameters in set 1.
  const std::array<VkDescriptorSet, 2> sets{
      draw.images.get().set(), draw.parameters.get().set()};
  vkCmdBindDescriptorSets(state.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          draw.pipeline.get().layout(), 0,
                          static_cast<uint32_t>(sets.size()), sets.data(), 0,
                          nullptr);
}

void TexturePreview::draw(const Image &value, ImVec2 available,
                          bool interactive) {
  const uint32_t slot = interactive ? 1 : 0;
  auto view = value.view;
  view.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view.baseMipLevel += static_cast<uint32_t>(mip_);
  view.baseArrayLayer += static_cast<uint32_t>(layer_);
  view.levelCount = view.layerCount = 1;
  const float width = static_cast<float>(
      std::max(1U, value.image.width >> std::min(view.baseMipLevel, 31U)));
  const float height = static_cast<float>(
      std::max(1U, value.image.height >> std::min(view.baseMipLevel, 31U)));
  available.x = std::max(1.0f, available.x);
  available.y = std::max(1.0f, available.y);
  if (!ImGui::BeginChild(interactive ? "##viewer_canvas" : "##thumbnail",
                         available, ImGuiChildFlags_None,
                         ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse)) {
    ImGui::EndChild();
    return;
  }
  const auto origin = ImGui::GetCursorScreenPos();
  available = ImGui::GetContentRegionAvail();
  available.x = std::max(1.0f, available.x);
  available.y = std::max(1.0f, available.y);
  ImGui::InvisibleButton("##image_canvas", available,
                         ImGuiButtonFlags_MouseButtonLeft |
                             ImGuiButtonFlags_MouseButtonMiddle);
  const bool hovered = ImGui::IsItemHovered();
  const bool clicked = ImGui::IsItemClicked();
  if (interactive) {
    ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
  }
  const float fitScale = std::min(available.x / width, available.y / height);
  float scale = !interactive || fit_ ? fitScale : zoom_;
  ImVec2 pan = interactive ? pan_ : ImVec2{};
  const ImVec2 center{origin.x + available.x * 0.5f,
                      origin.y + available.y * 0.5f};
  if (interactive && hovered && ImGui::GetIO().MouseWheel != 0.0f) {
    const float next =
        std::clamp(scale * std::pow(1.2f, ImGui::GetIO().MouseWheel),
                   std::min(fitScale, 0.01f), std::max(fitScale, 64.0f));
    const auto mouse = ImGui::GetMousePos();
    pan.x = mouse.x - center.x - (mouse.x - center.x - pan.x) * next / scale;
    pan.y = mouse.y - center.y - (mouse.y - center.y - pan.y) * next / scale;
    zoom_ = scale = next;
    fit_ = false;
  }
  if (interactive && ImGui::IsItemActive() &&
      ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
    pan.x += ImGui::GetIO().MouseDelta.x;
    pan.y += ImGui::GetIO().MouseDelta.y;
  }
  if (interactive) {
    pan_ = pan;
  }
  const ImVec2 position{center.x + pan.x - width * scale * 0.5f,
                        center.y + pan.y - height * scale * 0.5f};
  const ImVec2 end{position.x + width * scale, position.y + height * scale};
  auto &frame = *frames_.at(frame_index_);
  if (frame.stale) {
    // Only invalidate the slot whose fence has completed. Other frames may
    // still reference views of the previously inspected image.
    for (auto &oldView : frame.views) {
      oldView.destroy();
    }
    frame.stale = false;
  }
  auto &imageView = frame.views[slot];
  const auto &previous = imageView.desc();
  if (imageView.imageView() == VK_NULL_HANDLE || previous.image != view.image ||
      previous.format != view.format ||
      previous.baseMipLevel != view.baseMipLevel ||
      previous.baseArrayLayer != view.baseArrayLayer ||
      previous.aspectMask != view.aspectMask ||
      previous.components.r != view.components.r ||
      previous.components.g != view.components.g ||
      previous.components.b != view.components.b ||
      previous.components.a != view.components.a) {
    imageView.update(view);
  }
  const auto &sampler =
      linear_ && (value.features &
                  VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
          ? linear_sampler_
          : nearest_sampler_;
  const auto index = frame_index_ * 2 + slot;
  auto &imageSet = image_sets_.at(index);
  const PreviewTexture texture{imageView, sampler, value.layout};
  imageSet.texture(0, texture);
  imageSet.update();
  parameter_sets_.at(index).update();
  Parameters parameters{};
  parameters.channel = channel_;
  parameters.exposure = exposure_;
  parameters.depth = view.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT;
  parameters.toneMap = !parameters.depth && tone_map_;
  parameters.depthMin = depth_min_;
  parameters.depthMax = depth_max_;
  const bool linearColor =
      srgb_ && !parameters.depth && parameters.channel != 5;
  const auto outputFormat =
      static_cast<vk::Format>(render_pass_.desc().colors.at(0).format);
  const bool srgbOutput =
      std::string_view(vk::componentNumericFormat(outputFormat, 0)) == "SRGB";
  parameters.encodeSrgb =
      srgbOutput ? (linearColor ? 0 : -1) : (linearColor ? 1 : 0);
  frame.parameters[slot].update(parameters);

  auto &drawList = *ImGui::GetWindowDrawList();
  drawList.PushClipRect(origin,
                        {origin.x + available.x, origin.y + available.y}, true);
  const ImVec2 visibleMin{std::max(origin.x, position.x),
                          std::max(origin.y, position.y)};
  const ImVec2 visibleMax{std::min(origin.x + available.x, end.x),
                          std::min(origin.y + available.y, end.y)};
  if (visibleMin.x < visibleMax.x && visibleMin.y < visibleMax.y) {
    ImGui::RenderColorRectWithAlphaCheckerboard(
        &drawList, visibleMin, visibleMax, IM_COL32(0, 0, 0, 0),
        ImGui::GetFontSize() * 0.75f, ImVec2{}, 0.0f);
    drawList.AddCallback(bind, &draws_.at(index));
    const bool flip = flip_y_;
    drawList.AddImage(reinterpret_cast<ImTextureID>(imageSet.set()),
                      position, end, {0.0f, flip ? 1.0f : 0.0f},
                      {1.0f, flip ? 0.0f : 1.0f});
    drawList.AddCallback(ImDrawCallback_ResetRenderState, nullptr);
  }
  drawList.PopClipRect();

  if (hovered) {
    const auto mouse = ImGui::GetMousePos();
    const float u = (mouse.x - position.x) / (width * scale);
    const float v = (mouse.y - position.y) / (height * scale);
    if (interactive && u >= 0 && u < 1 && v >= 0 && v < 1) {
      const uint32_t x = static_cast<uint32_t>(u * width);
      const uint32_t y =
          std::min(static_cast<uint32_t>(height) - 1,
                   static_cast<uint32_t>((flip_y_ ? 1 - v : v) * height));
      ImGui::SetTooltip("Texel %u, %u | UV %.4f, %.4f | Zoom %.1f%%", x, y, u,
                        flip_y_ ? 1 - v : v,
                        scale * ImGui::GetIO().DisplayFramebufferScale.x *
                            100.0f);
      if (ImGui::IsItemClicked() &&
          (value.image.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0 &&
          texelFormat(view.format)) {
        requested_probe_ = Probe{selection_, view, originalView(), x, y};
      }
    } else if (!interactive) {
      ImGui::SetTooltip("Click to inspect channels, mip levels and layers");
    }
  }
  ImGui::EndChild();
  // Open in the Inspector's ID scope, not the thumbnail child's scope.
  if (!interactive && clicked) {
    ImGui::OpenPopup("Resource preview");
  }
}

void TexturePreview::render() {
  const auto current = image();
  if (!current) {
    ImGui::TextDisabled("Image unavailable.");
    return;
  }
  if (const auto error = validate(*current); !error.empty()) {
    ImGui::TextWrapped("%.*s", static_cast<int>(error.size()), error.data());
    return;
  }
  const auto view = originalView();
  if (inspected_.type != selection_.type ||
      inspected_.name != selection_.name || inspected_view_ != view) {
    inspected_ = selection_;
    inspected_view_ = view;
    for (auto &frame : frames_) {
      frame->stale = true;
    }
    mip_ = layer_ = channel_ = 0;
    exposure_ = 0;
    depth_min_ = 0;
    depth_max_ = 1;
    fit_ = true;
    pan_ = {};
    flip_y_ = false;
    const std::string_view type = vk::componentNumericFormat(
        static_cast<vk::Format>(current->view.format), 0);
    tone_map_ = type == "SFLOAT" || type == "UFLOAT";
    srgb_ = type == "SRGB" || type == "SFLOAT" || type == "UFLOAT";
    requested_probe_.reset();
    completed_probe_.reset();
    texel_.reset();
  }
  if (!pipeline_.valid()) {
    createPipeline();
  }
  if (ImGui::TreeNode("Display settings")) {
    controls(*current, false);
    ImGui::TreePop();
  }
  draw(*current,
       {ImGui::GetContentRegionAvail().x, ImGui::GetFontSize() * 14.0f}, false);
  const auto workSize = ImGui::GetMainViewport()->WorkSize;
  ImGui::SetNextWindowSize(
      {std::min(workSize.x * 0.85f, ImGui::GetFontSize() * 54.0f),
       std::min(workSize.y * 0.85f, ImGui::GetFontSize() * 42.0f)},
      ImGuiCond_Appearing);
  if (ImGui::BeginPopup("Resource preview", ImGuiWindowFlags_NoSavedSettings)) {
    ImGui::TextWrapped("%s", selection_.name.c_str());
    ImGui::TextDisabled(
        "%u x %u | %s", current->image.width, current->image.height,
        vk::to_string(static_cast<vk::Format>(current->view.format)).c_str());
    ImGui::Separator();
    controls(*current, true);
    if (texel_ && completed_probe_ &&
        completed_probe_->view.baseMipLevel ==
            current->view.baseMipLevel + mip_ &&
        completed_probe_->view.baseArrayLayer ==
            current->view.baseArrayLayer + layer_) {
      const auto &probe = *completed_probe_;
      ImGui::Text("Texel %u, %u: %.6g  %.6g  %.6g  %.6g", probe.x, probe.y,
                  (*texel_)[0], (*texel_)[1], (*texel_)[2], (*texel_)[3]);
      ImGui::TextDisabled("Raw normalized/float values, before swizzle and "
                          "display conversion.");
    } else if ((current->image.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0) {
      ImGui::TextDisabled("Texel readback requires TRANSFER_SRC image usage.");
    } else if (!texelFormat(current->view.format)) {
      ImGui::TextDisabled(
          "Texel readback is unavailable for this packed/compressed format.");
    } else {
      ImGui::TextDisabled("Click a texel to request asynchronous readback.");
    }
    ImGui::Separator();
    draw(*current, ImGui::GetContentRegionAvail(), true);
    ImGui::EndPopup();
  }
}

} // namespace vkr::ui
