#include "vkr/exec/graph.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <deque>

namespace vkr::exec {

Graph::Graph(std::string name) : name_(std::move(name)) {}

Graph::~Graph() { destroy(); }

void Graph::ensureBuilding() const {
  if (state_ != State::Building) {
    VKR_EXEC_ERROR("{} graph configuration is frozen; destroy the graph "
                   "before changing it",
                   name_);
  }
}

void Graph::ensureCreated() const {
  if (state_ != State::Created) {
    VKR_EXEC_ERROR("{} graph executed before create", name_);
  }
}

void Graph::addPass(std::unique_ptr<Pass> pass) {
  ensureBuilding();

  if (!pass) {
    VKR_EXEC_ERROR("Cannot add null {} graph pass", name_);
  }

  if (!pass->name().empty()) {
    validatePassNameAvailable(pass->name());
  }

  passes_.push_back(std::move(pass));
}

void Graph::addDependency(std::string producer, std::string consumer) {
  ensureBuilding();

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
}

void Graph::compile() {
  if (state_ == State::Created) {
    VKR_EXEC_ERROR("Destroy the {} graph before recompiling it", name_);
  }
  if (state_ == State::Compiled) {
    return;
  }

  const size_t passCount = passes_.size();
  std::unordered_map<std::string, size_t> indices{};
  indices.reserve(passCount);

  for (size_t index = 0; index < passCount; ++index) {
    const auto &name = passes_[index]->name();
    if (name.empty()) {
      VKR_EXEC_ERROR("{} graph pass at index {} has empty name", name_, index);
    }
    if (!indices.emplace(name, index).second) {
      VKR_EXEC_ERROR("{} graph pass '{}' is duplicated", name_, name);
    }
  }

  validate();

  std::vector<std::vector<size_t>> dependencies(passCount);
  auto addDependency = [&dependencies](size_t producer,
                                       size_t consumer) -> void {
    if (producer == consumer) {
      return;
    }
    auto &consumers = dependencies[producer];
    if (std::find(consumers.begin(), consumers.end(), consumer) ==
        consumers.end()) {
      consumers.push_back(consumer);
    }
  };

  for (const auto &[producerName, consumers] : manual_dependencies_) {
    const auto producer = indices.find(producerName);
    if (producer == indices.end()) {
      VKR_EXEC_ERROR("{} graph dependency references unknown producer '{}'",
                     name_, producerName);
    }

    for (const auto &consumerName : consumers) {
      const auto consumer = indices.find(consumerName);
      if (consumer == indices.end()) {
        VKR_EXEC_ERROR("{} graph dependency references unknown consumer '{}'",
                       name_, consumerName);
      }
      addDependency(producer->second, consumer->second);
    }
  }

  for (size_t producer = 0; producer < passCount; ++producer) {
    for (size_t consumer = 0; consumer < passCount; ++consumer) {
      if (producer == consumer) {
        continue;
      }
      for (const auto &written : passes_[producer]->writes()) {
        if (contains(passes_[consumer]->reads(), written)) {
          addDependency(producer, consumer);
        }
      }
    }
  }

  for (size_t first = 0; first < passCount; ++first) {
    for (size_t second = first + 1; second < passCount; ++second) {
      for (const auto &written : passes_[first]->writes()) {
        if (contains(passes_[second]->writes(), written)) {
          addDependency(first, second);
        }
      }
    }
  }

  std::vector<size_t> indegree(passCount, 0);
  for (const auto &consumers : dependencies) {
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

  std::vector<size_t> ordered{};
  ordered.reserve(passCount);
  while (!ready.empty()) {
    const size_t index = ready.front();
    ready.pop_front();
    ordered.push_back(index);
    for (const size_t consumer : dependencies[index]) {
      if (--indegree[consumer] == 0) {
        ready.push_back(consumer);
      }
    }
  }

  if (ordered.size() != passCount) {
    VKR_EXEC_ERROR("{} graph contains a dependency cycle", name_);
  }

  VKR_EXEC_INFO("{} graph compiled: passes={}", name_, passCount);
  ordered_passes_ = std::move(ordered);
  dependent_passes_ = std::move(dependencies);
  for (const auto &pass : passes_) {
    pass->configuration_locked_ = true;
  }
  state_ = State::Compiled;
}

void Graph::create() {
  if (state_ == State::Created) {
    VKR_EXEC_ERROR("{} graph is already created", name_);
  }
  if (state_ == State::Building) {
    compile();
  }

  size_t attempted = 0;
  try {
    for (const size_t index : ordered_passes_) {
      attempted++;
      passes_[index]->create();
    }
  } catch (...) {
    while (attempted > 0) {
      passes_[ordered_passes_[--attempted]]->destroy();
    }
    throw;
  }

  state_ = State::Created;
}

void Graph::destroy() noexcept {
  if (state_ == State::Created) {
    for (auto it = ordered_passes_.rbegin(); it != ordered_passes_.rend();
         ++it) {
      passes_[*it]->destroy();
    }
  }

  for (const auto &pass : passes_) {
    pass->configuration_locked_ = false;
  }
  ordered_passes_.clear();
  dependent_passes_.clear();
  state_ = State::Building;
}

void Graph::record() {
  ensureCreated();

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

auto Graph::executionOrder() const
    -> std::vector<std::reference_wrapper<const Pass>> {
  if (state_ == State::Building) {
    VKR_EXEC_ERROR("Compile the {} graph before inspecting its execution order",
                   name_);
  }

  std::vector<std::reference_wrapper<const Pass>> result{};
  result.reserve(ordered_passes_.size());
  for (const size_t index : ordered_passes_) {
    result.emplace_back(*passes_[index]);
  }
  return result;
}

auto Graph::dependencies(const Pass &pass) const
    -> std::vector<std::reference_wrapper<const Pass>> {
  if (state_ == State::Building) {
    VKR_EXEC_ERROR("Compile the {} graph before inspecting its dependencies",
                   name_);
  }

  const auto consumer = std::find_if(
      passes_.begin(), passes_.end(), [&pass](const auto &entry) -> bool {
        return std::addressof(*entry) == std::addressof(pass);
      });
  if (consumer == passes_.end()) {
    VKR_EXEC_ERROR("Pass '{}' does not belong to the {} graph", pass.name(),
                   name_);
  }
  const size_t consumerIndex = static_cast<size_t>(consumer - passes_.begin());

  std::vector<std::reference_wrapper<const Pass>> result{};
  for (const size_t producer : ordered_passes_) {
    const auto &consumers = dependent_passes_[producer];
    if (std::find(consumers.begin(), consumers.end(), consumerIndex) !=
        consumers.end()) {
      result.emplace_back(*passes_[producer]);
    }
  }
  return result;
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

auto Graph::contains(const std::vector<std::string> &values,
                     const std::string &target) -> bool {
  return std::find(values.begin(), values.end(), target) != values.end();
}

} // namespace vkr::exec
