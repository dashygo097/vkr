#include "vkr/util/runtime_path.hh"

#include <string>
#include <system_error>

#if defined(__APPLE__)
#include <cstring>
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#include <vector>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

auto normalizedPath(const std::filesystem::path &path)
    -> std::filesystem::path {
  std::error_code ec;
  auto normalized = std::filesystem::weakly_canonical(path, ec);
  if (!ec) {
    return normalized;
  }

  normalized = std::filesystem::absolute(path, ec);
  if (!ec) {
    return normalized.lexically_normal();
  }

  return path.lexically_normal();
}

} // namespace

namespace vkr::util {

auto executablePath() -> std::filesystem::path {
#if defined(__APPLE__)
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);

  std::string path(size, '\0');
  if (_NSGetExecutablePath(path.data(), &size) == 0) {
    path.resize(std::strlen(path.c_str()));
    return normalizedPath(path);
  }
#elif defined(__linux__)
  std::vector<char> path(4096);
  while (true) {
    const auto count = readlink("/proc/self/exe", path.data(), path.size());
    if (count < 0) {
      break;
    }

    if (static_cast<size_t>(count) < path.size()) {
      return normalizedPath(
          std::string(path.data(), static_cast<size_t>(count)));
    }

    path.resize(path.size() * 2);
  }
#elif defined(_WIN32)
  std::string path(MAX_PATH, '\0');
  while (true) {
    const auto count = GetModuleFileNameA(nullptr, path.data(),
                                          static_cast<DWORD>(path.size()));
    if (count == 0) {
      break;
    }

    if (count < path.size()) {
      path.resize(count);
      return normalizedPath(path);
    }

    path.resize(path.size() * 2);
  }
#endif

  return normalizedPath(std::filesystem::current_path());
}

auto executableDir() -> std::filesystem::path {
  auto path = executablePath();
  if (path.has_parent_path()) {
    return path.parent_path();
  }

  return normalizedPath(std::filesystem::current_path());
}

} // namespace vkr::util
