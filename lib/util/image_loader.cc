#define STB_IMAGE_IMPLEMENTATION
#include "vkr/util/image_loader.hh"
#include "vkr/logger.hh"
#include <limits>
#include <memory>
#include <stb_image.h>

namespace vkr::util {

auto loadImage(const std::filesystem::path &path, bool forceRgba) -> ImageData {
  int width{}, height{}, channels{};
  const std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels{
      stbi_load(path.string().c_str(), &width, &height, &channels,
                forceRgba ? STBI_rgb_alpha : 0),
      &stbi_image_free};
  if (!pixels || width <= 0 || height <= 0 || channels <= 0) {
    VKR_UTIL_ERROR("Failed to decode image '{}': {}", path.string(),
                   stbi_failure_reason() ? stbi_failure_reason()
                                         : "invalid dimensions");
  }

  ImageData result{};
  result.width = static_cast<uint32_t>(width);
  result.height = static_cast<uint32_t>(height);
  result.channels = forceRgba ? 4U : static_cast<uint32_t>(channels);
  const uint64_t size =
      uint64_t{result.width} * result.height * result.channels;
  if (size > std::numeric_limits<size_t>::max()) {
    VKR_UTIL_ERROR("Image '{}' exceeds addressable memory", path.string());
  }
  result.pixels.assign(pixels.get(), pixels.get() + static_cast<size_t>(size));
  return result;
}

} // namespace vkr::util
