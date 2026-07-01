#pragma once

#include "vkr/core/device.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace vkr::render {

class PipelineLibrary {
public:
  explicit PipelineLibrary(const core::Device &device);
  ~PipelineLibrary();

  PipelineLibrary(const PipelineLibrary &) = delete;
  auto operator=(const PipelineLibrary &) -> PipelineLibrary & = delete;

  auto create(const pipeline::GraphicsPipelineDesc &desc)
      -> pipeline::GraphicsPipeline &;

  auto update(const pipeline::GraphicsPipelineDesc &desc)
      -> pipeline::GraphicsPipeline &;

  void remove(const std::string &name);
  void clear();

  [[nodiscard]] auto contains(const std::string &name) const noexcept -> bool;

  [[nodiscard]] auto get(const std::string &name)
      -> pipeline::GraphicsPipeline &;

  [[nodiscard]] auto get(const std::string &name) const
      -> const pipeline::GraphicsPipeline &;

  [[nodiscard]] auto first() -> pipeline::GraphicsPipeline &;

  [[nodiscard]] auto first() const -> const pipeline::GraphicsPipeline &;

  [[nodiscard]] auto names() const -> std::vector<std::string>;

  [[nodiscard]] auto empty() const noexcept -> bool {
    return pipelines_.empty();
  }

  [[nodiscard]] auto size() const noexcept -> size_t {
    return pipelines_.size();
  }

private:
  const core::Device &device_;
  std::unordered_map<std::string, std::unique_ptr<pipeline::GraphicsPipeline>>
      pipelines_{};
};

} // namespace vkr::render
