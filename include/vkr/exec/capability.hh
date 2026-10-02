#pragma once

#include <cstdint>
#include <functional>
#include <optional>

namespace vkr::pipeline {
class GraphicsPipeline;
}

namespace vkr::exec {

class OffscreenTarget;

class PresentCapability {
public:
  virtual ~PresentCapability() = default;
  virtual void present() = 0;
};

class GraphicsPipelineCapability {
public:
  virtual ~GraphicsPipelineCapability() = default;

  [[nodiscard]] virtual auto editablePipeline() noexcept
      -> std::optional<std::reference_wrapper<pipeline::GraphicsPipeline>> = 0;
  [[nodiscard]] virtual auto editablePipeline() const noexcept -> std::optional<
      std::reference_wrapper<const pipeline::GraphicsPipeline>> = 0;
};

class RenderTargetCapability {
public:
  virtual ~RenderTargetCapability() = default;

  [[nodiscard]] virtual auto target(uint32_t frameIndex)
      -> OffscreenTarget & = 0;
  [[nodiscard]] virtual auto target(uint32_t frameIndex) const
      -> const OffscreenTarget & = 0;
};

} // namespace vkr::exec
