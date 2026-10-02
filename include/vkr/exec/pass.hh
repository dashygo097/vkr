#pragma once

#include "vkr/pipeline/graphics_pipeline.hh"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::exec {

enum class PassCapability : uint32_t {
  None = 0,
  Present = 1U << 0U,
  GraphicsPipeline = 1U << 1U,
};

[[nodiscard]] constexpr auto operator|(PassCapability lhs,
                                       PassCapability rhs) noexcept
    -> PassCapability {
  return static_cast<PassCapability>(static_cast<uint32_t>(lhs) |
                                     static_cast<uint32_t>(rhs));
}

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

class Pass {
public:
  explicit Pass(PassCapability capabilities = PassCapability::None)
      : capabilities_(capabilities) {}
  virtual ~Pass() = default;

  Pass(const Pass &) = delete;
  auto operator=(const Pass &) -> Pass & = delete;

  [[nodiscard]] auto name() const noexcept -> const std::string & {
    return name_;
  }

  [[nodiscard]] auto reads() const noexcept
      -> const std::vector<std::string> & {
    return reads_;
  }

  [[nodiscard]] auto writes() const noexcept
      -> const std::vector<std::string> & {
    return writes_;
  }

  auto setName(std::string name) -> Pass & {
    name_ = std::move(name);
    return *this;
  }

  auto setReads(std::vector<std::string> reads) -> Pass & {
    reads_ = std::move(reads);
    return *this;
  }

  auto setWrites(std::vector<std::string> writes) -> Pass & {
    writes_ = std::move(writes);
    return *this;
  }

  auto read(std::string resource) -> Pass & {
    reads_.push_back(std::move(resource));
    return *this;
  }

  auto write(std::string resource) -> Pass & {
    writes_.push_back(std::move(resource));
    return *this;
  }

  virtual void create() = 0;
  virtual void destroy() = 0;
  virtual void record() = 0;

  template <typename CapabilityT>
  [[nodiscard]] auto capability()
      -> std::optional<std::reference_wrapper<CapabilityT>> {
    if (!supports(capabilityType<CapabilityT>())) {
      return std::nullopt;
    }

    return dynamic_cast<CapabilityT &>(*this);
  }

  template <typename CapabilityT>
  [[nodiscard]] auto capability() const
      -> std::optional<std::reference_wrapper<const CapabilityT>> {
    if (!supports(capabilityType<CapabilityT>())) {
      return std::nullopt;
    }

    return dynamic_cast<const CapabilityT &>(*this);
  }

private:
  template <typename CapabilityT>
  [[nodiscard]] static constexpr auto capabilityType() noexcept
      -> PassCapability {
    static_assert(std::is_same_v<CapabilityT, PresentCapability> ||
                      std::is_same_v<CapabilityT, GraphicsPipelineCapability>,
                  "CapabilityT is not a registered pass capability");

    if constexpr (std::is_same_v<CapabilityT, PresentCapability>) {
      return PassCapability::Present;
    }

    return PassCapability::GraphicsPipeline;
  }

  [[nodiscard]] auto supports(PassCapability capability) const noexcept
      -> bool {
    return (static_cast<uint32_t>(capabilities_) &
            static_cast<uint32_t>(capability)) != 0;
  }

  // components
  PassCapability capabilities_{PassCapability::None};
  std::string name_{};
  std::vector<std::string> reads_{};
  std::vector<std::string> writes_{};
};

} // namespace vkr::exec
