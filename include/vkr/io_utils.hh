#pragma once

#include "./logger.hh"
#include <fstream>

namespace vkr {
static std::vector<char> fread_char(const std::string &path) {
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    VKR_CORE_ERROR("Failed to open file: {}", path);
  }
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();
  return buffer;
};

static std::vector<uint32_t> fread_uint32(const std::string &path) {
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open())
    VKR_CORE_ERROR("Failed to open file: {}", path);
  size_t size = static_cast<size_t>(file.tellg());
  std::vector<uint32_t> buf(size / 4);
  file.seekg(0);
  file.read(reinterpret_cast<char *>(buf.data()), size);
  return buf;
}

static std::string fread_string(const std::string &path) {
  std::ifstream f(path);
  if (!f.is_open())
    VKR_CORE_ERROR("Failed to open file: {}", path);
  return std::string((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
}

} // namespace vkr
