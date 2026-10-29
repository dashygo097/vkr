#include "vkr/exec/pass.hh"
#include "vkr/logger.hh"

namespace vkr::exec {

auto Pass::dependencies() const
    -> std::vector<std::reference_wrapper<const Pass>> {
  return {};
}

auto Pass::name(std::string name) -> Pass & {
  ensureConfigurable();
  name_ = std::move(name);
  return *this;
}

auto Pass::reads(std::vector<std::string> reads) -> Pass & {
  ensureConfigurable();
  reads_ = std::move(reads);
  return *this;
}

auto Pass::writes(std::vector<std::string> writes) -> Pass & {
  ensureConfigurable();
  writes_ = std::move(writes);
  return *this;
}

auto Pass::read(std::string resource) -> Pass & {
  ensureConfigurable();
  reads_.push_back(std::move(resource));
  return *this;
}

auto Pass::write(std::string resource) -> Pass & {
  ensureConfigurable();
  writes_.push_back(std::move(resource));
  return *this;
}

void Pass::ensureConfigurable() const {
  if (configuration_locked_) {
    VKR_EXEC_ERROR("Pass '{}' configuration is frozen; destroy the graph "
                   "before changing it",
                   name_);
  }
}

} // namespace vkr::exec
