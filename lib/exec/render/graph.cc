#include "vkr/exec/render/graph.hh"
#include "vkr/logger.hh"
#include <deque>
#include <string_view>

namespace vkr::exec {

void RenderGraph::addPass(std::unique_ptr<Pass> pass) {
  if (!pass) {
    VKR_EXEC_ERROR("Cannot add null render graph pass");
  }

  if (!pass->name().empty()) {
    validatePassNameAvailable(pass->name());
  }

  passes_.push_back(std::move(pass));
  dirty_ = true;
}

void RenderGraph::addDependency(std::string producer, std::string consumer) {
  if (producer.empty()) {
    VKR_EXEC_ERROR("Render graph dependency has empty producer");
  }

  if (consumer.empty()) {
    VKR_EXEC_ERROR("Render graph dependency has empty consumer");
  }

  if (producer == consumer) {
    VKR_EXEC_ERROR("Render graph pass '{}' cannot depend on itself", producer);
  }

  manual_dependencies_[std::move(producer)].push_back(std::move(consumer));
  dirty_ = true;
}

void RenderGraph::compile() {
  rebuildNameTable();
  validateDependencies();
  validatePresentationContract();

  const size_t passCount = passes_.size();

  compiled_dependencies_.clear();
  compiled_dependencies_.resize(passCount);

  for (const auto &[producerName, consumers] : manual_dependencies_) {
    const size_t producer = passIndex(producerName);

    for (const auto &consumerName : consumers) {
      const size_t consumer = passIndex(consumerName);
      addCompiledDependency(producer, consumer);
    }
  }

  buildResourceDependencies();

  std::vector<size_t> indegree(passCount, 0);

  for (const auto &consumers : compiled_dependencies_) {
    for (const size_t consumer : consumers) {
      indegree[consumer]++;
    }
  }

  std::deque<size_t> ready{};

  for (size_t i = 0; i < passCount; ++i) {
    if (indegree[i] == 0) {
      ready.push_back(i);
    }
  }

  ordered_passes_.clear();
  ordered_passes_.reserve(passCount);

  while (!ready.empty()) {
    const size_t index = ready.front();
    ready.pop_front();

    ordered_passes_.push_back(index);

    for (const size_t consumer : compiled_dependencies_[index]) {
      if (indegree[consumer] == 0) {
        VKR_EXEC_ERROR("Render graph internal indegree underflow");
      }

      indegree[consumer]--;

      if (indegree[consumer] == 0) {
        ready.push_back(consumer);
      }
    }
  }

  if (ordered_passes_.size() != passCount) {
    VKR_EXEC_ERROR("Render graph contains a dependency cycle");
  }

  dirty_ = false;

  VKR_EXEC_INFO("Render graph compiled: passes={}", passes_.size());
}

auto RenderGraph::create() -> void {
  if (dirty_) {
    compile();
  }

  for (const size_t index : ordered_passes_) {
    passes_[index]->create();
  }

  created_ = true;
}

auto RenderGraph::destroy() -> void {
  for (auto it = ordered_passes_.rbegin(); it != ordered_passes_.rend(); ++it) {
    passes_[*it]->destroy();
  }

  created_ = false;
}

auto RenderGraph::record() -> void {
  if (dirty_) {
    compile();
  }

  if (!created_) {
    create();
  }

  for (const size_t index : ordered_passes_) {
    passes_[index]->record();
  }
}

auto RenderGraph::present() -> void {
  if (dirty_) {
    compile();
  }

  if (!created_) {
    VKR_EXEC_ERROR("Render graph presented before create");
  }

  for (const size_t index : ordered_passes_) {
    passes_[index]->present();
  }
}

auto RenderGraph::afterFrame() -> void {
  if (dirty_) {
    compile();
  }

  if (!created_) {
    VKR_EXEC_ERROR("Render graph afterFrame called before create");
  }

  for (const size_t index : ordered_passes_) {
    passes_[index]->afterFrame();
  }
}

auto RenderGraph::passes() -> std::vector<std::reference_wrapper<Pass>> {
  std::vector<std::reference_wrapper<Pass>> result{};
  result.reserve(passes_.size());

  for (const auto &pass : passes_) {
    result.emplace_back(*pass);
  }

  return result;
}

auto RenderGraph::passes() const
    -> std::vector<std::reference_wrapper<const Pass>> {
  std::vector<std::reference_wrapper<const Pass>> result{};
  result.reserve(passes_.size());

  for (const auto &pass : passes_) {
    result.emplace_back(*pass);
  }

  return result;
}

auto RenderGraph::rebuildNameTable() -> void {
  pass_indices_.clear();
  pass_indices_.reserve(passes_.size());

  for (size_t i = 0; i < passes_.size(); ++i) {
    const auto &name = passes_[i]->name();

    if (name.empty()) {
      VKR_EXEC_ERROR("Render graph pass at index {} has empty name", i);
    }

    auto [_, inserted] = pass_indices_.emplace(name, i);
    if (!inserted) {
      VKR_EXEC_ERROR("Render graph pass '{}' is duplicated", name);
    }
  }
}

auto RenderGraph::validatePassNameAvailable(std::string_view name) const
    -> void {
  if (name.empty()) {
    VKR_EXEC_ERROR("Render graph pass name cannot be empty");
  }

  for (const auto &pass : passes_) {
    if (pass->name() == name) {
      VKR_EXEC_ERROR("Render graph pass '{}' already exists",
                     std::string(name));
    }
  }
}

auto RenderGraph::validateDependencies() const -> void {
  for (const auto &[producer, consumers] : manual_dependencies_) {
    if (pass_indices_.find(producer) == pass_indices_.end()) {
      VKR_EXEC_ERROR("Render graph dependency references unknown producer '{}'",
                     producer);
    }

    for (const auto &consumer : consumers) {
      if (pass_indices_.find(consumer) == pass_indices_.end()) {
        VKR_EXEC_ERROR(
            "Render graph dependency references unknown consumer '{}'",
            consumer);
      }
    }
  }
}

auto RenderGraph::validatePresentationContract() const -> void {
  size_t presenterCount = 0;
  bool writesSwapchain = false;

  for (const auto &pass : passes_) {
    if (pass->presentsToSwapchain()) {
      presenterCount++;
    }

    for (const auto &written : pass->writes()) {
      if (written == "swapchain") {
        writesSwapchain = true;
      }
    }
  }

  if (presenterCount > 1) {
    VKR_EXEC_ERROR("Render graph has {} PresentPass nodes; only one "
                   "swapchain presenter is allowed",
                   presenterCount);
  }

  if (presenterCount == 0) {
    VKR_EXEC_ERROR("Render graph has no PresentPass");
  }

  if (!writesSwapchain) {
    VKR_EXEC_ERROR("Render graph has a PresentPass but no pass writes "
                   "'swapchain'");
  }
}

auto RenderGraph::addCompiledDependency(size_t producer, size_t consumer)
    -> void {
  if (producer == consumer) {
    return;
  }

  auto &consumers = compiled_dependencies_[producer];

  for (const size_t existing : consumers) {
    if (existing == consumer) {
      return;
    }
  }

  consumers.push_back(consumer);
}

auto RenderGraph::buildResourceDependencies() -> void {
  const size_t passCount = passes_.size();

  for (size_t producer = 0; producer < passCount; ++producer) {
    const auto &writes = passes_[producer]->writes();

    for (size_t consumer = 0; consumer < passCount; ++consumer) {
      if (producer == consumer) {
        continue;
      }

      const auto &reads = passes_[consumer]->reads();

      for (const auto &written : writes) {
        if (contains(reads, written)) {
          addCompiledDependency(producer, consumer);
        }
      }
    }
  }

  for (size_t first = 0; first < passCount; ++first) {
    const auto &firstWrites = passes_[first]->writes();

    for (size_t second = first + 1; second < passCount; ++second) {
      const auto &secondWrites = passes_[second]->writes();

      for (const auto &written : firstWrites) {
        if (contains(secondWrites, written)) {
          addCompiledDependency(first, second);
        }
      }
    }
  }
}

auto RenderGraph::passIndex(std::string_view name) const -> size_t {
  auto it = pass_indices_.find(std::string(name));

  if (it == pass_indices_.end()) {
    VKR_EXEC_ERROR("Render graph pass '{}' does not exist", std::string(name));
  }

  return it->second;
}

auto RenderGraph::contains(const std::vector<std::string> &values,
                           const std::string &target) -> bool {
  for (const auto &value : values) {
    if (value == target) {
      return true;
    }
  }

  return false;
}

} // namespace vkr::exec
