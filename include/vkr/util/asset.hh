#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace vkr::util {

enum class AssetRootKind {
  App,
  User,
};

struct AssetDesc {
  std::string appRoot{"assets"};
  std::string userRoot{"."};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return !appRoot.empty() && !userRoot.empty();
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("appRoot", appRoot);
    ar("userRoot", userRoot);
  }
};

class AssetSystem {
public:
  explicit AssetSystem(const AssetDesc &desc);

  AssetSystem(const AssetSystem &) = delete;
  auto operator=(const AssetSystem &) -> AssetSystem & = delete;

  [[nodiscard]] auto desc() const noexcept -> const AssetDesc & {
    return desc_;
  }

  [[nodiscard]] auto resolve(std::string_view logicalPath) const
      -> std::filesystem::path;

  [[nodiscard]] auto tryResolve(std::string_view logicalPath) const
      -> std::optional<std::filesystem::path>;

  [[nodiscard]] auto resolveApp(std::string_view path) const
      -> std::filesystem::path {
    return resolveWithPrefix("app://", path);
  }

  [[nodiscard]] auto resolveUser(std::string_view path) const
      -> std::filesystem::path {
    return resolveWithPrefix("user://", path);
  }

  [[nodiscard]] auto readText(std::string_view logicalPath) const
      -> std::string;

private:
  // components
  AssetDesc desc_{};

  // helpers
  [[nodiscard]] static auto startsWith(std::string_view value,
                                       std::string_view prefix) noexcept
      -> bool;

  [[nodiscard]] static auto
  isSafeRelativePath(const std::filesystem::path &path) noexcept -> bool;

  [[nodiscard]] auto rootPath(AssetRootKind kind) const
      -> std::filesystem::path;

  [[nodiscard]] auto resolveFromRoot(const std::filesystem::path &root,
                                     std::string_view relativePath) const
      -> std::optional<std::filesystem::path>;

  [[nodiscard]] auto resolveWithPrefix(std::string_view prefix,
                                       std::string_view path) const
      -> std::filesystem::path;
};

} // namespace vkr::util
