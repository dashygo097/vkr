#pragma once

#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::render {

class Renderer;

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

  [[nodiscard]] auto desc() const noexcept -> const RenderGraphPassDesc & {
    return desc_;
  }

  [[nodiscard]] auto name() const noexcept -> const std::string & {
    return desc_.name;
  }

  [[nodiscard]] auto reads() const noexcept
      -> const std::vector<std::string> & {
    return desc_.reads;
  }

  [[nodiscard]] auto writes() const noexcept
      -> const std::vector<std::string> & {
    return desc_.writes;
  }

  auto read(std::string resource) -> RenderGraphPass & {
    desc_.reads.push_back(std::move(resource));
    return *this;
  }

  auto write(std::string resource) -> RenderGraphPass & {
    desc_.writes.push_back(std::move(resource));
    return *this;
  }

  virtual void create() = 0;
  virtual void destroy() = 0;
  virtual void update(const RenderGraphPassDesc &desc) = 0;
  virtual void record() = 0;

protected:
  void setDesc(RenderGraphPassDesc desc) { desc_ = std::move(desc); }

private:
  // components
  RenderGraphPassDesc desc_;
};

} // namespace vkr::render
