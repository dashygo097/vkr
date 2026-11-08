#pragma once

#include "vkr/util/obj_loader.hh"
#include <array>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::scene {

enum class VertexSemantic {
  Position,
  Color,
  Normal,
  TexCoord,
};

struct VertexImportDesc {
  struct Attribute {
    VertexSemantic semantic;
    VkFormat format;
    size_t offset;
  };

  std::vector<Attribute> attributes{};

  auto attribute(VertexSemantic semantic, VkFormat format, size_t offset)
      -> VertexImportDesc &;

  template <typename VertexType>
  [[nodiscard]] auto pack(const std::vector<util::ObjVertex> &source) const
      -> std::vector<VertexType> {
    static_assert(std::is_standard_layout_v<VertexType>,
                  "Imported vertex data must have a standard layout");
    static_assert(std::is_trivially_copyable_v<VertexType>,
                  "Imported vertex data must be trivially copyable");
    static_assert(std::is_default_constructible_v<VertexType>,
                  "Imported vertices must support value initialization");

    const auto mappings = prepare(sizeof(VertexType));
    std::vector<VertexType> vertices(source.size());
    for (size_t index = 0; index < source.size(); ++index) {
      for (const auto &mapping : mappings) {
        const auto bytes = encode(source[index], mapping);
        std::memcpy(reinterpret_cast<std::byte *>(&vertices[index]) +
                        mapping.attribute.offset,
                    bytes.data(), mapping.components * mapping.componentBytes);
      }
    }
    return vertices;
  }

private:
  enum class Encoding { Float, UNorm, SNorm, UInt, SInt };

  struct Mapping {
    Attribute attribute;
    Encoding encoding;
    size_t components;
    size_t componentBytes;
    std::array<uint8_t, 4> channels{};
  };

  [[nodiscard]] auto prepare(size_t stride) const -> std::vector<Mapping>;
  [[nodiscard]] static auto encode(const util::ObjVertex &source,
                                   const Mapping &mapping)
      -> std::array<std::byte, 32>;
};

} // namespace vkr::scene
