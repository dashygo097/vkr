#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace vkr::util {

struct ImageData {
  uint32_t width{};
  uint32_t height{};
  uint32_t channels{};
  std::vector<uint8_t> pixels{};
};

[[nodiscard]] auto loadImage(const std::filesystem::path &path,
                             bool forceRgba = true) -> ImageData;

} // namespace vkr::util
