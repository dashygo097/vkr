#pragma once

#include "vkr/pipeline/graphics_pipeline.hh"
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::render {

struct RenderGraphPassDesc {
  std::string name{};
  std::vector<std::string> reads{};
  std::vector<std::string> writes{};
};

class RenderGraphPass {
public:
  explicit RenderGraphPass() = default;
  virtual ~RenderGraphPass() = default;

  RenderGraphPass(const RenderGraphPass &) = delete;
  auto operator=(const RenderGraphPass &) -> RenderGraphPass & = delete;

  [[nodiscard]] virtual auto desc() const noexcept
      -> const RenderGraphPassDesc & = 0;

  [[nodiscard]] auto name() const noexcept -> const std::string & {
    return desc().name;
  }

  [[nodiscard]] auto reads() const noexcept
      -> const std::vector<std::string> & {
    return desc().reads;
  }

  [[nodiscard]] auto writes() const noexcept
      -> const std::vector<std::string> & {
    return desc().writes;
  }

  auto read(std::string resource) -> RenderGraphPass & {
    mutableDesc().reads.push_back(std::move(resource));
    return *this;
  }

  auto write(std::string resource) -> RenderGraphPass & {
    mutableDesc().writes.push_back(std::move(resource));
    return *this;
  }

  virtual void create() = 0;
  virtual void destroy() = 0;
  virtual void update(const RenderGraphPassDesc &desc) = 0;
  virtual void record() = 0;

  [[nodiscard]] virtual auto editablePipeline() noexcept
      -> pipeline::GraphicsPipeline * {
    return nullptr;
  }

  [[nodiscard]] virtual auto editablePipeline() const noexcept
      -> const pipeline::GraphicsPipeline * {
    return nullptr;
  }

protected:
  void setDesc(RenderGraphPassDesc desc) { mutableDesc() = std::move(desc); }
  [[nodiscard]] virtual auto mutableDesc() noexcept -> RenderGraphPassDesc & = 0;
};

} // namespace vkr::render
