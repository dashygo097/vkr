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

    auto pass = std::make_unique<PassT>(std::forward<Args>(args)...);
    auto &ref = *pass;
    addPass(std::move(pass));
    return ref;
  }

  void addPass(std::unique_ptr<Pass> pass);
  void addDependency(std::string producer, std::string consumer);

  void compile();
  void create();
  void destroy();
  void record();
  void present();

  [[nodiscard]] auto passes() -> std::vector<std::reference_wrapper<Pass>>;
  [[nodiscard]] auto passes() const
      -> std::vector<std::reference_wrapper<const Pass>>;

  template <typename PassT>
  [[nodiscard]] auto getPass(std::string_view name)
      -> std::optional<std::reference_wrapper<PassT>> {
    const auto index = passIndex(name);

    try {
      return dynamic_cast<PassT &>(*passes_[index]);
    } catch (const std::bad_cast &) {
      return std::nullopt;
    }
  }

  template <typename PassT>
  [[nodiscard]] auto getPass(std::string_view name) const
      -> std::optional<std::reference_wrapper<const PassT>> {
    const auto index = passIndex(name);

    try {
      return dynamic_cast<const PassT &>(*passes_[index]);
    } catch (const std::bad_cast &) {
      return std::nullopt;
    }
  }

protected:
  explicit Graph(std::string name);

  virtual void validateContract() const = 0;

  [[nodiscard]] auto passStorage() const noexcept
      -> const std::vector<std::unique_ptr<Pass>> & {
    return passes_;
  }

private:
  // components
  std::string name_{};
  std::vector<std::unique_ptr<Pass>> passes_{};
  std::unordered_map<std::string, size_t> pass_indices_{};
  std::unordered_map<std::string, std::vector<std::string>>
      manual_dependencies_{};
  std::vector<std::vector<size_t>> compiled_dependencies_{};
  std::vector<size_t> ordered_passes_{};

  // states
  bool dirty_{true};
  bool created_{false};

  // helpers
  void rebuildNameTable();
  void validatePassNameAvailable(std::string_view name) const;
  void validateDependencies() const;
  void addCompiledDependency(size_t producer, size_t consumer);
  void buildResourceDependencies();

  [[nodiscard]] auto passIndex(std::string_view name) const -> size_t;
  [[nodiscard]] static auto contains(const std::vector<std::string> &values,
                                     const std::string &target) -> bool;
};

} // namespace vkr::exec
