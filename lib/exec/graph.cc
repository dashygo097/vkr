#include "vkr/exec/graph.hh"
#include "vkr/logger.hh"
#include <deque>

namespace vkr::exec {

Graph::Graph(std::string name) : name_(std::move(name)) {}

Graph::~Graph() { destroy(); }

void Graph::addPass(std::unique_ptr<Pass> pass) {
  if (!pass) {
    VKR_EXEC_ERROR("Cannot add null {} graph pass", name_);
  }

  if (!pass->name().empty()) {
    validatePassNameAvailable(pass->name());
  }

  passes_.push_back(std::move(pass));
  dirty_ = true;
}

void Graph::addDependency(std::string producer, std::string consumer) {
  if (producer.empty()) {
    VKR_EXEC_ERROR("{} graph dependency has empty producer", name_);
  }

  if (consumer.empty()) {
    VKR_EXEC_ERROR("{} graph dependency has empty consumer", name_);
  }

  if (producer == consumer) {
    VKR_EXEC_ERROR("{} graph pass '{}' cannot depend on itself", name_,
                   producer);
  }

  manual_dependencies_[std::move(producer)].push_back(std::move(consumer));
  dirty_ = true;
}

void Graph::compile() {
  rebuildNameTable();
  validateDependencies();
  validateContract();

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
  for (size_t index = 0; index < passCount; ++index) {
    if (indegree[index] == 0) {
      ready.push_back(index);
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
        VKR_EXEC_ERROR("{} graph internal indegree underflow", name_);
      }

      indegree[consumer]--;
      if (indegree[consumer] == 0) {
        ready.push_back(consumer);
      }
    }
  }

  if (ordered_passes_.size() != passCount) {
    VKR_EXEC_ERROR("{} graph contains a dependency cycle", name_);
  }

  dirty_ = false;
  VKR_EXEC_INFO("{} graph compiled: passes={}", name_, passes_.size());
}

void Graph::create() {
  if (dirty_) {
    compile();
  }

  for (const size_t index : ordered_passes_) {
    passes_[index]->create();
  }

  created_ = true;
}

void Graph::destroy() {
  for (auto it = ordered_passes_.rbegin(); it != ordered_passes_.rend(); ++it) {
    passes_[*it]->destroy();
  }

  created_ = false;
}

void Graph::record() {
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

auto Graph::passes() -> std::vector<std::reference_wrapper<Pass>> {
  std::vector<std::reference_wrapper<Pass>> result{};
  result.reserve(passes_.size());

  for (const auto &pass : passes_) {
    result.emplace_back(*pass);
  }

  return result;
}

auto Graph::passes() const -> std::vector<std::reference_wrapper<const Pass>> {
  std::vector<std::reference_wrapper<const Pass>> result{};
  result.reserve(passes_.size());

  for (const auto &pass : passes_) {
    result.emplace_back(*pass);
  }

  return result;
}

void Graph::present() {
  if (dirty_) {
    compile();
  }

  if (!created_) {
    VKR_EXEC_ERROR("{} graph presented before create", name_);
  }

  for (const size_t index : ordered_passes_) {
    const auto capability = passes_[index]->capability<PresentCapability>();
    if (capability) {
      capability->get().present();
    }
  }
}

void Graph::rebuildNameTable() {
  pass_indices_.clear();
  pass_indices_.reserve(passes_.size());

  for (size_t index = 0; index < passes_.size(); ++index) {
    const auto &name = passes_[index]->name();

    if (name.empty()) {
      VKR_EXEC_ERROR("{} graph pass at index {} has empty name", name_, index);
    }

    auto [_, inserted] = pass_indices_.emplace(name, index);
    if (!inserted) {
      VKR_EXEC_ERROR("{} graph pass '{}' is duplicated", name_, name);
    }
  }
}

void Graph::validatePassNameAvailable(std::string_view name) const {
  if (name.empty()) {
    VKR_EXEC_ERROR("{} graph pass name cannot be empty", name_);
  }

  for (const auto &pass : passes_) {
    if (pass->name() == name) {
      VKR_EXEC_ERROR("{} graph pass '{}' already exists", name_,
                     std::string(name));
    }
  }
}

void Graph::validateDependencies() const {
  for (const auto &[producer, consumers] : manual_dependencies_) {
    if (pass_indices_.find(producer) == pass_indices_.end()) {
      VKR_EXEC_ERROR("{} graph dependency references unknown producer '{}'",
                     name_, producer);
    }

    for (const auto &consumer : consumers) {
      if (pass_indices_.find(consumer) == pass_indices_.end()) {
        VKR_EXEC_ERROR("{} graph dependency references unknown consumer '{}'",
                       name_, consumer);
      }
    }
  }
}

void Graph::addCompiledDependency(size_t producer, size_t consumer) {
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

void Graph::buildResourceDependencies() {
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

auto Graph::passIndex(std::string_view name) const -> size_t {
  const auto it = pass_indices_.find(std::string(name));

  if (it == pass_indices_.end()) {
    VKR_EXEC_ERROR("{} graph pass '{}' does not exist", name_,
                   std::string(name));
  }

  return it->second;
}

auto Graph::contains(const std::vector<std::string> &values,
                     const std::string &target) -> bool {
  for (const auto &value : values) {
    if (value == target) {
      return true;
    }
  }

  return false;
}

} // namespace vkr::exec
