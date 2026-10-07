#pragma once

#include "vkr/exec/pass.hh"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vkr::exec {

class Graph {
public:
  virtual ~Graph();

  Graph(const Graph &) = delete;
  auto operator=(const Graph &) -> Graph & = delete;

  template <typename PassT, typename... Args>
  auto addPass(Args &&...args) -> PassT & {
    static_assert(std::is_base_of_v<Pass, PassT>,
                  "PassT must derive from Pass");

    ensureBuilding();
    auto pass = std::make_unique<PassT>(std::forward<Args>(args)...);
    auto &ref = *pass;
    addPass(std::move(pass));
    return ref;
  }

  void addPass(std::unique_ptr<Pass> pass);
  void addDependency(std::string producer, std::string consumer);

  virtual void compile();
  void create();
  virtual void destroy() noexcept;
  void record();

  [[nodiscard]] auto passes() -> std::vector<std::reference_wrapper<Pass>>;
  [[nodiscard]] auto passes() const
      -> std::vector<std::reference_wrapper<const Pass>>;

  [[nodiscard]] auto executionOrder() const
      -> std::vector<std::reference_wrapper<const Pass>>;
  [[nodiscard]] auto dependencies(const Pass &pass) const
      -> std::vector<std::reference_wrapper<const Pass>>;

  template <typename PassT>
  [[nodiscard]] auto getPass(std::string_view name)
      -> std::optional<std::reference_wrapper<PassT>> {
    for (const auto &pass : passes_) {
      if (pass->name() == name) {
        try {
          return dynamic_cast<PassT &>(*pass);
        } catch (const std::bad_cast &) {
          return std::nullopt;
        }
      }
    }
    return std::nullopt;
  }

  template <typename PassT>
  [[nodiscard]] auto getPass(std::string_view name) const
      -> std::optional<std::reference_wrapper<const PassT>> {
    for (const auto &pass : passes_) {
      if (pass->name() == name) {
        try {
          return dynamic_cast<const PassT &>(*pass);
        } catch (const std::bad_cast &) {
          return std::nullopt;
        }
      }
    }
    return std::nullopt;
  }

protected:
  explicit Graph(std::string name);

  void ensureBuilding() const;
  void ensureCreated() const;
  virtual void validate() const = 0;

  [[nodiscard]] auto passStorage() const noexcept
      -> const std::vector<std::unique_ptr<Pass>> & {
    return passes_;
  }

private:
  enum class State {
    Building,
    Compiled,
    Created,
  };

  // components
  std::string name_{};
  std::vector<std::unique_ptr<Pass>> passes_{};
  std::unordered_map<std::string, std::vector<std::string>>
      manual_dependencies_{};
  std::vector<size_t> ordered_passes_{};
  std::vector<std::vector<size_t>> dependent_passes_{};

  // states
  State state_{State::Building};

  // helpers
  void validatePassNameAvailable(std::string_view name) const;
  [[nodiscard]] static auto contains(const std::vector<std::string> &values,
                                     const std::string &target) -> bool;
};

} // namespace vkr::exec
