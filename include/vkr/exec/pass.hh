#pragma once

#include "vkr/pipeline/graphics_pipeline.hh"
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::exec {

class Pass {
public:
  explicit Pass() = default;
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

  virtual void present() {}
  virtual void afterFrame() {}

  [[nodiscard]] virtual auto presentsToSwapchain() const noexcept -> bool {
    return false;
  }

  [[nodiscard]] virtual auto editablePipeline() noexcept
      -> std::optional<std::reference_wrapper<pipeline::GraphicsPipeline>> {
    return std::nullopt;
  }

  [[nodiscard]] virtual auto editablePipeline() const noexcept -> std::optional<
      std::reference_wrapper<const pipeline::GraphicsPipeline>> {
    return std::nullopt;
  }

private:
  // components
  std::string name_{};
  std::vector<std::string> reads_{};
  std::vector<std::string> writes_{};
};

} // namespace vkr::exec
