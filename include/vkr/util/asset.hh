#pragma once

#include "vkr/logger.hh"
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace vkr::util {

enum class AssetRootKind {
  Engine,
  App,
  User,
};

struct AssetDesc {
  std::string engineRoot{std::string{ENGINE_ASSETS_DIR} + "assets/"};
  std::string appRoot{"assets"};
  std::string userRoot{"."};
  bool preferAppRoot{true};
  bool allowAbsolutePath{false};
  bool requireExists{true};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return !engineRoot.empty() && !appRoot.empty() && !userRoot.empty();
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("engineRoot", engineRoot);
    ar("appRoot", appRoot);
    ar("userRoot", userRoot);
  }
};

class AssetSystem {
public:
  explicit AssetSystem(const AssetDesc &desc) : desc_(std::move(desc)) {
    if (!desc_.isValid()) {
      VKR_UTIL_ERROR("Invalid asset descriptor");
    }

    VKR_UTIL_INFO(
        "Asset engine root: {}",
        std::filesystem::path(desc_.engineRoot).lexically_normal().string());
    VKR_UTIL_INFO(
        "Asset app root: {}",
        std::filesystem::path(desc_.appRoot).lexically_normal().string());
    VKR_UTIL_INFO(
        "Asset user root: {}",
        std::filesystem::path(desc_.userRoot).lexically_normal().string());
  }

  [[nodiscard]] auto desc() const noexcept -> const AssetDesc & {
    return desc_;
  }

  [[nodiscard]] auto resolve(std::string_view logicalPath) const
      -> std::filesystem::path {
    auto result = tryResolve(logicalPath);

    if (!result) {
      VKR_UTIL_ERROR("Asset not found: {}", std::string(logicalPath));
    }

    return *result;
  }

  [[nodiscard]] auto tryResolve(std::string_view logicalPath) const
      -> std::optional<std::filesystem::path> {
    auto raw = std::string(logicalPath);

    if (startsWith(raw, "engine://")) {
      return resolveFromRoot(rootPath(AssetRootKind::Engine), raw.substr(9));
    }

    if (startsWith(raw, "app://")) {
      return resolveFromRoot(rootPath(AssetRootKind::App), raw.substr(6));
    }

    if (startsWith(raw, "user://")) {
      return resolveFromRoot(rootPath(AssetRootKind::User), raw.substr(7));
    }

    auto path = std::filesystem::path(raw);

    if (path.is_absolute()) {
      if (!desc_.allowAbsolutePath) {
        VKR_UTIL_WARN("Absolute asset path rejected: {}", path.string());
        return std::nullopt;
      }

      auto normalized = path.lexically_normal();
      if (!desc_.requireExists || std::filesystem::exists(normalized)) {
        return normalized;
      }

      return std::nullopt;
    }

    if (desc_.preferAppRoot) {
      if (auto app = resolveFromRoot(rootPath(AssetRootKind::App), raw)) {
        return app;
      }
      return resolveFromRoot(rootPath(AssetRootKind::Engine), raw);
    }

    if (auto engine = resolveFromRoot(rootPath(AssetRootKind::Engine), raw)) {
      return engine;
    }

    return resolveFromRoot(rootPath(AssetRootKind::App), raw);
  }

  [[nodiscard]] auto resolveEngine(std::string_view path) const
      -> std::filesystem::path {
    return resolveWithPrefix("engine://", path);
  }

  [[nodiscard]] auto resolveApp(std::string_view path) const
      -> std::filesystem::path {
    return resolveWithPrefix("app://", path);
  }

  [[nodiscard]] auto resolveUser(std::string_view path) const
      -> std::filesystem::path {
    return resolveWithPrefix("user://", path);
  }

  [[nodiscard]] auto readText(std::string_view logicalPath) const
      -> std::string {
    auto path = resolve(logicalPath);

    std::ifstream file(path);
    if (!file) {
      VKR_UTIL_ERROR("Failed to open text asset: {}", path.string());
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
  }

private:
  [[nodiscard]] static auto startsWith(std::string_view value,
                                       std::string_view prefix) noexcept
      -> bool {
    return value.size() >= prefix.size() &&
           value.substr(0, prefix.size()) == prefix;
  }

  [[nodiscard]] static auto
  isSafeRelativePath(const std::filesystem::path &path) noexcept -> bool {
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

  [[nodiscard]] auto rootPath(AssetRootKind kind) const
      -> std::filesystem::path {
    switch (kind) {
    case AssetRootKind::Engine:
      return std::filesystem::path(desc_.engineRoot).lexically_normal();
    case AssetRootKind::App:
      return std::filesystem::path(desc_.appRoot).lexically_normal();
    case AssetRootKind::User:
    default:
      return std::filesystem::path(desc_.userRoot).lexically_normal();
    }
  }

  [[nodiscard]] auto resolveFromRoot(const std::filesystem::path &root,
                                     std::string_view relativePath) const
      -> std::optional<std::filesystem::path> {
    auto rel = std::filesystem::path(std::string(relativePath));

    if (!isSafeRelativePath(rel)) {
      VKR_UTIL_WARN("Unsafe asset path rejected: {}", rel.string());
      return std::nullopt;
    }

    auto candidate = (root / rel).lexically_normal();

    if (!desc_.requireExists || std::filesystem::exists(candidate)) {
      return candidate;
    }

    return std::nullopt;
  }

  [[nodiscard]] auto resolveWithPrefix(std::string_view prefix,
                                       std::string_view path) const
      -> std::filesystem::path {
    std::string logical;
    logical.reserve(prefix.size() + path.size());
    logical.append(prefix);
    logical.append(path);
    return resolve(logical);
  }

  AssetDesc desc_;
};

} // namespace vkr::util
