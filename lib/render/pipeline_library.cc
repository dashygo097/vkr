#include "vkr/render/pipeline_library.hh"
#include "vkr/logger.hh"
#include <stdexcept>
#include <utility>

namespace vkr::render {

PipelineLibrary::PipelineLibrary(const core::Device &device)
    : device_(device) {}

PipelineLibrary::~PipelineLibrary() { clear(); }

auto PipelineLibrary::create(const pipeline::GraphicsPipelineDesc &desc)
    -> pipeline::GraphicsPipeline & {
  if (!desc.isValid()) {
    throw std::runtime_error("cannot create invalid graphics pipeline");
  }

  auto pipeline = std::make_unique<pipeline::GraphicsPipeline>(device_);
  pipeline->update(desc);

  if (!pipeline->valid()) {
    throw std::runtime_error("failed to create graphics pipeline '" +
                             desc.name + "'");
  }

  auto [it, inserted] =
      pipelines_.insert_or_assign(desc.name, std::move(pipeline));

  VKR_RENDER_INFO("{} graphics pipeline '{}'",
                  inserted ? "Created" : "Replaced", desc.name);

  return *it->second;
}

auto PipelineLibrary::update(const pipeline::GraphicsPipelineDesc &desc)
    -> pipeline::GraphicsPipeline & {
  auto it = pipelines_.find(desc.name);

  if (it == pipelines_.end()) {
    return create(desc);
  }

  it->second->update(desc);

  if (!it->second->valid()) {
    throw std::runtime_error("failed to update graphics pipeline '" +
                             desc.name + "'");
  }

  VKR_RENDER_INFO("Updated graphics pipeline '{}'", desc.name);
  return *it->second;
}

void PipelineLibrary::remove(const std::string &name) {
  auto it = pipelines_.find(name);

  if (it == pipelines_.end()) {
    return;
  }

  pipelines_.erase(it);
  VKR_RENDER_INFO("Removed graphics pipeline '{}'", name);
}

void PipelineLibrary::clear() { pipelines_.clear(); }

auto PipelineLibrary::contains(const std::string &name) const noexcept -> bool {
  return pipelines_.find(name) != pipelines_.end();
}

auto PipelineLibrary::get(const std::string &name)
    -> pipeline::GraphicsPipeline & {
  auto it = pipelines_.find(name);

  if (it == pipelines_.end()) {
    throw std::runtime_error("graphics pipeline not found: " + name);
  }

  return *it->second;
}

auto PipelineLibrary::get(const std::string &name) const
    -> const pipeline::GraphicsPipeline & {
  auto it = pipelines_.find(name);

  if (it == pipelines_.end()) {
    throw std::runtime_error("graphics pipeline not found: " + name);
  }

  return *it->second;
}

auto PipelineLibrary::first() -> pipeline::GraphicsPipeline & {
  if (pipelines_.empty()) {
    throw std::runtime_error("pipeline library is empty");
  }

  return *pipelines_.begin()->second;
}

auto PipelineLibrary::first() const -> const pipeline::GraphicsPipeline & {
  if (pipelines_.empty()) {
    throw std::runtime_error("pipeline library is empty");
  }

  return *pipelines_.begin()->second;
}

auto PipelineLibrary::names() const -> std::vector<std::string> {
  std::vector<std::string> result{};
  result.reserve(pipelines_.size());

  for (const auto &[name, _] : pipelines_) {
    result.push_back(name);
  }

  return result;
}

} // namespace vkr::render
