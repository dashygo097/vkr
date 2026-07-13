#pragma once

#include "vkr/pipeline/graphics_pipeline.hh"
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::resource {
class OffscreenTarget;
}

namespace vkr::render {

class RenderGraphPass {
public:
  explicit RenderGraphPass() = default;
  virtual ~RenderGraphPass() = default;

  RenderGraphPass(const RenderGraphPass &) = delete;
  auto operator=(const RenderGraphPass &) -> RenderGraphPass & = delete;

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

  auto setName(std::string name) -> RenderGraphPass & {
    name_ = std::move(name);
    return *this;
  }

  auto setReads(std::vector<std::string> reads) -> RenderGraphPass & {
    reads_ = std::move(reads);
    return *this;
  }

  auto setWrites(std::vector<std::string> writes) -> RenderGraphPass & {
    writes_ = std::move(writes);
    return *this;
  }

  auto read(std::string resource) -> RenderGraphPass & {
    reads_.push_back(std::move(resource));
    return *this;
  }

  auto write(std::string resource) -> RenderGraphPass & {
    writes_.push_back(std::move(resource));
    return *this;
  }

  virtual void create() = 0;
  virtual void destroy() = 0;
  virtual void record() = 0;

  [[nodiscard]] virtual auto editablePipeline() noexcept
      -> pipeline::GraphicsPipeline * {
    return nullptr;
  }

  [[nodiscard]] virtual auto editablePipeline() const noexcept
      -> const pipeline::GraphicsPipeline * {
    return nullptr;
  }

private:
  std::string name_{};
  std::vector<std::string> reads_{};
  std::vector<std::string> writes_{};
};

class OffscreenRenderPass {
public:
  virtual ~OffscreenRenderPass() = default;

  [[nodiscard]] virtual auto target() -> vkr::resource::OffscreenTarget & = 0;
  [[nodiscard]] virtual auto target() const
      -> const vkr::resource::OffscreenTarget & = 0;
};

} // namespace vkr::render
