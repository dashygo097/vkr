#pragma once

#include "vkr/logger.hh"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <limits>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>
#include <type_traits>
#include <utility>

namespace vkr::util {

template <typename T> struct IsGlmVec3 : std::false_type {};

template <> struct IsGlmVec3<glm::vec3> : std::true_type {};

template <typename T, typename = void> struct HasIsValid : std::false_type {};

template <typename T>
struct HasIsValid<T, std::void_t<decltype(std::declval<const T &>().isValid())>>
    : std::true_type {};

class TomlSaveArchive {
public:
  TomlSaveArchive() = default;

  [[nodiscard]] auto take() -> toml::table { return std::move(table_); }

  auto operator()(std::string_view key, bool &value) -> void {
    table_.insert_or_assign(std::string(key), value);
  }

  template <typename T,
            std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>,
                             int> = 0>
  auto operator()(std::string_view key, T &value) -> void {
    table_.insert_or_assign(std::string(key), static_cast<int64_t>(value));
  }

  template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
  auto operator()(std::string_view key, T &value) -> void {
    table_.insert_or_assign(std::string(key), static_cast<double>(value));
  }

  template <typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
  auto operator()(std::string_view key, T &value) -> void {
    table_.insert_or_assign(std::string(key), static_cast<int64_t>(value));
  }

  auto operator()(std::string_view key, std::string &value) -> void {
    table_.insert_or_assign(std::string(key), value);
  }

  auto operator()(std::string_view key, glm::vec3 &value) -> void {
    table_.insert_or_assign(std::string(key),
                            toml::array{static_cast<double>(value.x),
                                        static_cast<double>(value.y),
                                        static_cast<double>(value.z)});
  }

  template <typename T,
            std::enable_if_t<!std::is_arithmetic_v<T> && !std::is_enum_v<T> &&
                                 !std::is_same_v<T, std::string> &&
                                 !IsGlmVec3<T>::value,
                             int> = 0>
  auto operator()(std::string_view key, T &value) -> void {
    TomlSaveArchive child;
    value.serialize(child);
    table_.insert_or_assign(std::string(key), child.take());
  }

private:
  toml::table table_;
};

class TomlLoadArchive {
public:
  explicit TomlLoadArchive(const toml::table &table) : table_(table) {}

  auto operator()(std::string_view key, bool &value) const -> void {
    auto k = std::string(key);
    if (auto next = table_[k].value<bool>()) {
      value = *next;
    }
  }

  template <typename T,
            std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>,
                             int> = 0>
  auto operator()(std::string_view key, T &value) const -> void {
    auto k = std::string(key);
    auto next = table_[k].value<int64_t>();
    if (!next) {
      return;
    }

    if constexpr (std::is_unsigned_v<T>) {
      if (*next >= 0 &&
          static_cast<uint64_t>(*next) <=
              static_cast<uint64_t>(std::numeric_limits<T>::max())) {
        value = static_cast<T>(*next);
      }
    } else {
      if (*next >= static_cast<int64_t>(std::numeric_limits<T>::min()) &&
          *next <= static_cast<int64_t>(std::numeric_limits<T>::max())) {
        value = static_cast<T>(*next);
      }
    }
  }

  template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
  auto operator()(std::string_view key, T &value) const -> void {
    auto k = std::string(key);
    if (auto next = table_[k].value<double>()) {
      value = static_cast<T>(*next);
      return;
    }
    if (auto next = table_[k].value<int64_t>()) {
      value = static_cast<T>(*next);
    }
  }

  template <typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
  auto operator()(std::string_view key, T &value) const -> void {
    auto k = std::string(key);
    if (auto next = table_[k].value<int64_t>()) {
      value = static_cast<T>(*next);
    }
  }

  auto operator()(std::string_view key, std::string &value) const -> void {
    auto k = std::string(key);
    if (auto next = table_[k].value<std::string>()) {
      value = *next;
    }
  }

  auto operator()(std::string_view key, glm::vec3 &value) const -> void {
    auto k = std::string(key);
    auto *arr = table_[k].as_array();
    if (!arr || arr->size() != 3) {
      return;
    }

    value.x = readFloat(arr->get(0), value.x);
    value.y = readFloat(arr->get(1), value.y);
    value.z = readFloat(arr->get(2), value.z);
  }

  template <typename T,
            std::enable_if_t<!std::is_arithmetic_v<T> && !std::is_enum_v<T> &&
                                 !std::is_same_v<T, std::string> &&
                                 !IsGlmVec3<T>::value,
                             int> = 0>
  auto operator()(std::string_view key, T &value) const -> void {
    auto k = std::string(key);
    auto *child = table_[k].as_table();
    if (!child) {
      return;
    }

    TomlLoadArchive archive(*child);
    value.serialize(archive);
  }

private:
  [[nodiscard]] static auto readFloat(const toml::node *node, float fallback)
      -> float {
    if (!node) {
      return fallback;
    }
    if (auto value = node->value<double>()) {
      return static_cast<float>(*value);
    }
    if (auto value = node->value<int64_t>()) {
      return static_cast<float>(*value);
    }
    return fallback;
  }

  const toml::table &table_;
};

template <typename T>
auto saveTomlFile(const std::filesystem::path &path, const T &value) -> bool {
  T next = value;

  TomlSaveArchive archive;
  next.serialize(archive);

  auto root = archive.take();

  std::ofstream file(path);
  if (!file) {
    VKR_UTIL_WARN("Failed to open TOML file for writing: {}", path.string());
    return false;
  }

  file << root;
  if (!file.good()) {
    VKR_UTIL_WARN("Failed to write TOML file: {}", path.string());
    return false;
  }

  VKR_UTIL_INFO("Saved TOML file: {}", path.string());
  return true;
}

template <typename T>
auto loadTomlFile(const std::filesystem::path &path, T &value) -> bool {
  try {
    auto root = toml::parse_file(path.string());

    T next = value;
    TomlLoadArchive archive(root);
    next.serialize(archive);

    if constexpr (HasIsValid<T>::value) {
      if (!next.isValid()) {
        VKR_UTIL_WARN("Invalid TOML data: {}", path.string());
        return false;
      }
    }

    value = next;
    VKR_UTIL_INFO("Loaded TOML file: {}", path.string());
    return true;
  } catch (const toml::parse_error &e) {
    VKR_UTIL_WARN("Failed to parse TOML file '{}': {}", path.string(),
                  e.description());
    return false;
  }
}

} // namespace vkr::util
