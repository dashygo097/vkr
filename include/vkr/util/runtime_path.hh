#pragma once

#include <filesystem>

namespace vkr::util {

[[nodiscard]] auto executablePath() -> std::filesystem::path;
[[nodiscard]] auto executableDir() -> std::filesystem::path;

} // namespace vkr::util
