#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::core {
class Device;
}

namespace vkr::pipe {

struct VertexInputDesc {
  std::vector<VkVertexInputBindingDescription> bindings{};
  std::vector<VkVertexInputAttributeDescription> attributes{};

  auto binding(uint32_t index, uint32_t stride,
               VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX)
      -> VertexInputDesc &;

  template <typename VertexType>
  auto binding(uint32_t index,
               VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX)
      -> VertexInputDesc & {
    static_assert(std::is_standard_layout_v<VertexType>,
                  "Vertex data must have a standard layout");
    static_assert(std::is_trivially_copyable_v<VertexType>,
                  "Vertex data must be trivially copyable");
    static_assert(sizeof(VertexType) <= std::numeric_limits<uint32_t>::max(),
                  "Vertex stride exceeds the Vulkan representation");
    return binding(index, static_cast<uint32_t>(sizeof(VertexType)), rate);
  }

  auto attribute(uint32_t location, VkFormat format, size_t offset,
                 uint32_t binding = 0) -> VertexInputDesc &;

  [[nodiscard]] static auto none() -> VertexInputDesc { return {}; }
  [[nodiscard]] auto isEmpty() const noexcept -> bool {
    return bindings.empty() && attributes.empty();
  }
  [[nodiscard]] auto isValid() const noexcept -> bool;
  void validate(const core::Device &device) const;
  [[nodiscard]] auto createInfo() const noexcept
      -> VkPipelineVertexInputStateCreateInfo;
};

} // namespace vkr::pipe
