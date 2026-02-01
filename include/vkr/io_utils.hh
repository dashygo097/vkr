#pragma once

#include "./logger.hh"
#include <fstream>

namespace vkr {
static std::vector<char> read_file(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    VKR_CORE_ERROR("Failed to open file: {}", filename);
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
};

} // namespace vkr
