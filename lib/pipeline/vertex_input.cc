#include "vkr/pipeline/vertex_input.hh"
#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <vulkan/vulkan_format_traits.hpp>

namespace vkr::pipe {

auto VertexInputDesc::binding(uint32_t index, uint32_t stride,
                              VkVertexInputRate rate) -> VertexInputDesc & {
  bindings.push_back({index, stride, rate});
  return *this;
}

auto VertexInputDesc::attribute(uint32_t location, VkFormat format,
                                size_t offset, uint32_t binding)
    -> VertexInputDesc & {
  if (offset > std::numeric_limits<uint32_t>::max()) {
    VKR_PIPE_ERROR("Vertex attribute offset exceeds the Vulkan representation");
  }
  attributes.push_back(
      {location, binding, format, static_cast<uint32_t>(offset)});
  return *this;
}

auto VertexInputDesc::isValid() const noexcept -> bool {
  for (size_t index = 0; index < bindings.size(); ++index) {
    const auto &binding = bindings[index];
    if (binding.stride == 0 ||
        (binding.inputRate != VK_VERTEX_INPUT_RATE_VERTEX &&
         binding.inputRate != VK_VERTEX_INPUT_RATE_INSTANCE)) {
      return false;
    }
    for (size_t previous = 0; previous < index; ++previous) {
      if (bindings[previous].binding == binding.binding) {
        return false;
      }
    }
  }
  for (size_t index = 0; index < attributes.size(); ++index) {
    const auto &attribute = attributes[index];
    const auto binding =
        std::find_if(bindings.begin(), bindings.end(),
                     [&attribute](const auto &value) -> bool {
                       return value.binding == attribute.binding;
                     });
    if (binding == bindings.end() || attribute.format == VK_FORMAT_UNDEFINED ||
        attribute.offset >= binding->stride) {
      return false;
    }
    for (size_t previous = 0; previous < index; ++previous) {
      if (attributes[previous].location == attribute.location) {
        return false;
      }
    }
  }
  return true;
}

void VertexInputDesc::validate(const core::Device &device) const {
  if (!isValid()) {
    VKR_PIPE_ERROR("Vertex input requires unique bindings/locations, valid "
                   "rates, and attributes within declared bindings");
  }
  const auto &limits = device.physicalDeviceproperties().limits;
  if (bindings.size() > limits.maxVertexInputBindings ||
      attributes.size() > limits.maxVertexInputAttributes) {
    VKR_PIPE_ERROR("Vertex input exceeds device binding/attribute limits");
  }
  for (const auto &binding : bindings) {
    if (binding.binding >= limits.maxVertexInputBindings ||
        binding.stride > limits.maxVertexInputBindingStride) {
      VKR_PIPE_ERROR("Vertex binding {} has an unsupported index or stride {}",
                     binding.binding, binding.stride);
    }
  }
  std::vector<uint32_t> locations;
  locations.reserve(attributes.size());
  for (const auto &attribute : attributes) {
    if (attribute.location >= limits.maxVertexInputAttributes ||
        attribute.offset > limits.maxVertexInputAttributeOffset) {
      VKR_PIPE_ERROR(
          "Vertex attribute {} exceeds device location/offset limits",
          attribute.location);
    }
    if ((device.formatProperties(attribute.format).bufferFeatures &
         VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) == 0) {
      VKR_PIPE_ERROR("Format {} cannot be used for vertex attributes",
                     static_cast<int>(attribute.format));
    }
    const auto format = static_cast<vk::Format>(attribute.format);
    const auto bytes = vk::blockSize(format);
    const auto binding =
        std::find_if(bindings.begin(), bindings.end(),
                     [&attribute](const auto &value) -> bool {
                       return value.binding == attribute.binding;
                     });
    if (bytes == 0 || bytes > binding->stride - attribute.offset) {
      VKR_PIPE_ERROR("Vertex attribute {} exceeds binding {} stride",
                     attribute.location, attribute.binding);
    }
    const uint32_t span =
        vk::componentBits(format, 0) == 64 && vk::componentCount(format) > 2
            ? 2U
            : 1U;
    if (span > limits.maxVertexInputAttributes - attribute.location) {
      VKR_PIPE_ERROR("Vertex attribute {} spans beyond the device limit",
                     attribute.location);
    }
    for (uint32_t index = 0; index < span; ++index) {
      const auto location = attribute.location + index;
      if (std::find(locations.begin(), locations.end(), location) !=
          locations.end()) {
        VKR_PIPE_ERROR("Vertex attribute locations overlap at {}", location);
      }
      locations.push_back(location);
    }
  }
}

auto VertexInputDesc::createInfo() const noexcept
    -> VkPipelineVertexInputStateCreateInfo {
  VkPipelineVertexInputStateCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  info.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
  info.pVertexBindingDescriptions =
      bindings.empty() ? nullptr : bindings.data();
  info.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributes.size());
  info.pVertexAttributeDescriptions =
      attributes.empty() ? nullptr : attributes.data();
  return info;
}

} // namespace vkr::pipe
