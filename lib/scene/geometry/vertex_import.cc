#include "vkr/scene/geometry/vertex_import.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <cmath>
#include <glm/gtc/packing.hpp>
#include <limits>
#include <string_view>
#include <vulkan/vulkan_format_traits.hpp>

namespace vkr::scene {
namespace {

template <typename T>
void store(std::array<std::byte, 32> &bytes, size_t offset, T value) {
  std::memcpy(bytes.data() + offset, &value, sizeof(T));
}

template <typename T> auto integer(float value) -> T {
  if (static_cast<double>(value) < std::numeric_limits<T>::lowest() ||
      static_cast<double>(value) > std::numeric_limits<T>::max()) {
    VKR_RES_ERROR("Imported vertex value {} exceeds the integer format range",
                  value);
  }
  return static_cast<T>(value);
}

} // namespace

auto VertexImportDesc::attribute(VertexSemantic semantic, VkFormat format,
                                 size_t offset) -> VertexImportDesc & {
  attributes.push_back({semantic, format, offset});
  return *this;
}

auto VertexImportDesc::prepare(size_t stride) const -> std::vector<Mapping> {
  if (attributes.empty() || stride == 0) {
    VKR_RES_ERROR("Vertex import requires attributes and a nonzero stride");
  }

  std::vector<Mapping> mappings;
  mappings.reserve(attributes.size());
  for (const auto &attribute : attributes) {
    switch (attribute.semantic) {
    case VertexSemantic::Position:
    case VertexSemantic::Color:
    case VertexSemantic::Normal:
    case VertexSemantic::TexCoord:
      break;
    default:
      VKR_RES_ERROR("Unsupported imported vertex semantic");
    }

    if (attribute.format < VK_FORMAT_R8_UNORM ||
        attribute.format > VK_FORMAT_R64G64B64A64_SFLOAT) {
      VKR_RES_ERROR("Unsupported vertex import format {}",
                    static_cast<int>(attribute.format));
    }
    const auto format = static_cast<vk::Format>(attribute.format);
    const auto components = vk::componentCount(format);
    const auto bits = vk::componentBits(format, 0);
    const std::string_view numeric = vk::componentNumericFormat(format, 0);
    if (vk::packed(format) != 0 || components == 0 || components > 4 ||
        (bits != 8 && bits != 16 && bits != 32 && bits != 64)) {
      VKR_RES_ERROR("Vertex import requires unpacked numeric components");
    }

    Encoding encoding;
    if (numeric == "SFLOAT" && bits >= 16) {
      encoding = Encoding::Float;
    } else if (numeric == "UNORM" && bits <= 16) {
      encoding = Encoding::UNorm;
    } else if (numeric == "SNORM" && bits <= 16) {
      encoding = Encoding::SNorm;
    } else if (numeric == "UINT" && bits <= 32) {
      encoding = Encoding::UInt;
    } else if (numeric == "SINT" && bits <= 32) {
      encoding = Encoding::SInt;
    } else {
      VKR_RES_ERROR("Unsupported vertex import numeric format {} / {} bits",
                    numeric, bits);
    }

    Mapping mapping{attribute, encoding, components, size_t{bits} / 8};
    for (uint8_t index = 0; index < components; ++index) {
      if (vk::componentBits(format, index) != bits ||
          numeric != vk::componentNumericFormat(format, index)) {
        VKR_RES_ERROR("Vertex import requires uniform component types");
      }
      const std::string_view name = vk::componentName(format, index);
      const auto channel = std::string_view("RGBA").find(name);
      if (name.size() != 1 || channel == std::string_view::npos) {
        VKR_RES_ERROR("Unsupported vertex import component {}", name);
      }
      mapping.channels[index] = static_cast<uint8_t>(channel);
    }
    const auto bytes = mapping.components * mapping.componentBytes;
    if (attribute.offset > stride || bytes > stride - attribute.offset) {
      VKR_RES_ERROR("Imported attribute at offset {} exceeds vertex stride {}",
                    attribute.offset, stride);
    }
    for (const auto &previous : mappings) {
      const auto previousEnd = previous.attribute.offset +
                               previous.components * previous.componentBytes;
      if (attribute.offset < previousEnd &&
          previous.attribute.offset < attribute.offset + bytes) {
        VKR_RES_ERROR("Imported vertex attributes overlap at offsets {} and {}",
                      previous.attribute.offset, attribute.offset);
      }
    }
    mappings.push_back(mapping);
  }
  return mappings;
}

auto VertexImportDesc::encode(const util::ObjVertex &source,
                              const Mapping &mapping)
    -> std::array<std::byte, 32> {
  glm::vec4 value{};
  switch (mapping.attribute.semantic) {
  case VertexSemantic::Position:
    value = glm::vec4(source.pos, 1.0f);
    break;
  case VertexSemantic::Color:
    value = glm::vec4(source.color, 1.0f);
    break;
  case VertexSemantic::Normal:
    value = glm::vec4(source.normal, 0.0f);
    break;
  case VertexSemantic::TexCoord:
    value = glm::vec4(source.texCoord, 0.0f, 0.0f);
    break;
  }

  std::array<std::byte, 32> bytes{};
  for (size_t component = 0; component < mapping.components; ++component) {
    const auto scalar = value[mapping.channels[component]];
    if (!std::isfinite(scalar)) {
      VKR_RES_ERROR("Imported vertex contains a non-finite value");
    }
    const auto offset = component * mapping.componentBytes;
    switch (mapping.encoding) {
    case Encoding::Float:
      if (mapping.componentBytes == 2) {
        if (std::abs(scalar) > 65504.0f) {
          VKR_RES_ERROR("Imported vertex value exceeds the float16 range");
        }
        store(bytes, offset, glm::packHalf1x16(scalar));
      } else if (mapping.componentBytes == 4) {
        store(bytes, offset, scalar);
      } else {
        store(bytes, offset, static_cast<double>(scalar));
      }
      break;
    case Encoding::UNorm: {
      const auto maximum = mapping.componentBytes == 1 ? 255.0 : 65535.0;
      const auto encoded = std::round(
          std::clamp(static_cast<double>(scalar), 0.0, 1.0) * maximum);
      if (mapping.componentBytes == 1) {
        store(bytes, offset, static_cast<uint8_t>(encoded));
      } else {
        store(bytes, offset, static_cast<uint16_t>(encoded));
      }
      break;
    }
    case Encoding::SNorm: {
      const auto maximum = mapping.componentBytes == 1 ? 127.0 : 32767.0;
      const auto encoded = std::round(
          std::clamp(static_cast<double>(scalar), -1.0, 1.0) * maximum);
      if (mapping.componentBytes == 1) {
        store(bytes, offset, static_cast<int8_t>(encoded));
      } else {
        store(bytes, offset, static_cast<int16_t>(encoded));
      }
      break;
    }
    case Encoding::UInt:
      if (mapping.componentBytes == 1) {
        store(bytes, offset, integer<uint8_t>(scalar));
      } else if (mapping.componentBytes == 2) {
        store(bytes, offset, integer<uint16_t>(scalar));
      } else {
        store(bytes, offset, integer<uint32_t>(scalar));
      }
      break;
    case Encoding::SInt:
      if (mapping.componentBytes == 1) {
        store(bytes, offset, integer<int8_t>(scalar));
      } else if (mapping.componentBytes == 2) {
        store(bytes, offset, integer<int16_t>(scalar));
      } else {
        store(bytes, offset, integer<int32_t>(scalar));
      }
      break;
    }
  }
  return bytes;
}

} // namespace vkr::scene
