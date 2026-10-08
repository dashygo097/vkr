#pragma once

#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>

namespace vkr::exec {

class Pass {
public:
  Pass() = default;
  virtual ~Pass() = default;

  Pass(const Pass &) = delete;
  auto operator=(const Pass &) -> Pass & = delete;

  [[nodiscard]] auto name() const noexcept -> const std::string & {
    return name_;
  }

  [[nodiscard]] auto reads() const noexcept
      -> const std::vector<std::string> & {
    return reads_;
  }

  [[nodiscard]] auto writes() const noexcept
      -> const std::vector<std::string> & {
    return writes_;
  }

  auto setName(std::string name) -> Pass &;
  auto setReads(std::vector<std::string> reads) -> Pass &;
  auto setWrites(std::vector<std::string> writes) -> Pass &;
  auto read(std::string resource) -> Pass &;
  auto write(std::string resource) -> Pass &;

  virtual void create() = 0;
  virtual void destroy() noexcept = 0;
  virtual void record() = 0;

  template <typename CapabilityT>
  [[nodiscard]] auto capability() noexcept
      -> std::optional<std::reference_wrapper<CapabilityT>> {
    static_assert(std::is_polymorphic_v<CapabilityT>,
                  "CapabilityT must be a polymorphic interface");

    try {
      return std::ref(dynamic_cast<CapabilityT &>(*this));
    } catch (const std::bad_cast &) {
      return std::nullopt;
    }
  }

  template <typename CapabilityT>
  [[nodiscard]] auto capability() const noexcept
      -> std::optional<std::reference_wrapper<const CapabilityT>> {
    static_assert(std::is_polymorphic_v<CapabilityT>,
                  "CapabilityT must be a polymorphic interface");

    try {
      return std::cref(dynamic_cast<const CapabilityT &>(*this));
    } catch (const std::bad_cast &) {
      return std::nullopt;
    }
  }

protected:
  void ensureConfigurable() const;

private:
  friend class Graph;

  // components
  std::string name_{};
  std::vector<std::string> reads_{};
  std::vector<std::string> writes_{};

  // states
  bool configuration_locked_{false};
};

} // namespace vkr::exec
