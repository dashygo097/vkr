#pragma once

#include "vkr/exec/compute/pass.hh"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vkr::exec {

class ComputeGraph {
public:
  ComputeGraph() = default;
  ~ComputeGraph() { destroy(); }

  ComputeGraph(const ComputeGraph &) = delete;
  auto operator=(const ComputeGraph &) -> ComputeGraph & = delete;

  auto addPass(ComputeExecutor &executor, const core::Device &device)
      -> ComputePass &;
  void addPass(std::unique_ptr<ComputePass> pass);
  void addDependency(std::string producer, std::string consumer);

  void compile();
  void create();
  void destroy();
  void record();

  [[nodiscard]] auto passes()
      -> std::vector<std::reference_wrapper<ComputePass>>;
  [[nodiscard]] auto passes() const
      -> std::vector<std::reference_wrapper<const ComputePass>>;

  [[nodiscard]] auto getPass(std::string_view name)
      -> std::optional<std::reference_wrapper<ComputePass>>;
  [[nodiscard]] auto getPass(std::string_view name) const
      -> std::optional<std::reference_wrapper<const ComputePass>>;

private:
  std::vector<std::unique_ptr<ComputePass>> passes_{};
  std::unordered_map<std::string, size_t> pass_indices_{};
  std::unordered_map<std::string, std::vector<std::string>>
      manual_dependencies_{};
  std::vector<std::vector<size_t>> compiled_dependencies_{};
  std::vector<size_t> ordered_passes_{};
  bool dirty_{true};
  bool created_{false};

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
