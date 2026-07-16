#include "vkr/util/asset.hh"
#include "vkr/logger.hh"
#include <fstream>
#include <sstream>

namespace vkr::util {

AssetSystem::AssetSystem(const AssetDesc &desc) : desc_(std::move(desc)) {
  if (!desc_.isValid()) {
    VKR_UTIL_ERROR("Invalid asset descriptor");
  }

  VKR_UTIL_INFO(
      "Asset app root: {}",
      std::filesystem::path(desc_.appRoot).lexically_normal().string());
  VKR_UTIL_INFO(
      "Asset user root: {}",
      std::filesystem::path(desc_.userRoot).lexically_normal().string());
}

auto AssetSystem::resolve(std::string_view logicalPath) const
    -> std::filesystem::path {
  auto result = tryResolve(logicalPath);

  if (!result) {
    VKR_UTIL_ERROR("Asset not found: {}", std::string(logicalPath));
  }

  return *result;
}

auto AssetSystem::tryResolve(std::string_view logicalPath) const
    -> std::optional<std::filesystem::path> {
  auto raw = std::string(logicalPath);

  if (startsWith(raw, "app://")) {
    return resolveFromRoot(rootPath(AssetRootKind::App), raw.substr(6));
  }

  if (startsWith(raw, "user://")) {
    return resolveFromRoot(rootPath(AssetRootKind::User), raw.substr(7));
  }

  auto path = std::filesystem::path(raw);

  if (path.is_absolute()) {
    auto normalized = path.lexically_normal();

    if (std::filesystem::exists(normalized)) {
      return normalized;
    }

    return std::nullopt;
  }

  if (auto app = resolveFromRoot(rootPath(AssetRootKind::App), raw)) {
    return app;
  }

  return std::nullopt;
}

auto AssetSystem::readText(std::string_view logicalPath) const -> std::string {
  auto path = resolve(logicalPath);

  std::ifstream file(path);
  if (!file) {
    VKR_UTIL_ERROR("Failed to open text asset: {}", path.string());
  }

  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

auto AssetSystem::startsWith(std::string_view value,
                             std::string_view prefix) noexcept -> bool {
  return value.size() >= prefix.size() &&
         value.substr(0, prefix.size()) == prefix;
}

auto AssetSystem::isSafeRelativePath(const std::filesystem::path &path) noexcept
    -> bool {
  if (path.empty() || path.is_absolute()) {
    return false;
  }

  for (const auto &part : path) {
    if (part == "..") {
      return false;
    }
  }

  return true;
}

auto AssetSystem::rootPath(AssetRootKind kind) const -> std::filesystem::path {
  switch (kind) {
  case AssetRootKind::App:
    return std::filesystem::path(desc_.appRoot).lexically_normal();
  case AssetRootKind::User:
    return std::filesystem::path(desc_.userRoot).lexically_normal();
  }

  return std::filesystem::path(desc_.userRoot).lexically_normal();
}

auto AssetSystem::resolveFromRoot(const std::filesystem::path &root,
                                  std::string_view relativePath) const
    -> std::optional<std::filesystem::path> {
  auto rel = std::filesystem::path(std::string(relativePath));

  if (!isSafeRelativePath(rel)) {
    VKR_UTIL_WARN("Unsafe asset path rejected: {}", rel.string());
    return std::nullopt;
  }

  auto candidate = (root / rel).lexically_normal();

  if (std::filesystem::exists(candidate)) {
    return candidate;
  }

  return std::nullopt;
}

auto AssetSystem::resolveWithPrefix(std::string_view prefix,
                                    std::string_view path) const
    -> std::filesystem::path {
  std::string logical;
  logical.reserve(prefix.size() + path.size());
  logical.append(prefix);
  logical.append(path);
  return resolve(logical);
}

} // namespace vkr::util
