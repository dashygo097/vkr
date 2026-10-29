#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/sync/fence.hh"
#include "vkr/logger.hh"
#include "vkr/pipeline/compute_pipeline.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include <functional>
#include <optional>
#include <string_view>
#include <vector>

namespace vkr::res {
class Buffer;
class BufferSlice;
} // namespace vkr::res

namespace vkr::exec {

class Profiler;

class ComputeExecutor {
public:
  explicit ComputeExecutor(const core::Device &device,
                           const core::CommandPool &commandPool);
  ~ComputeExecutor() = default;

  ComputeExecutor(const ComputeExecutor &) = delete;
  auto operator=(const ComputeExecutor &) -> ComputeExecutor & = delete;

  void begin();
  void submitAndWait();
  void end();
  void setProfiler(Profiler &profiler) noexcept;
  void clearProfiler() noexcept;

  [[nodiscard]] auto commandBuffer() const -> VkCommandBuffer;

  void bindPipeline(const pipe::ComputePipeline &pipeline);
  void bindPipeline(const pipe::ComputePipeline &pipeline,
                    pipe::DescriptorSet &set, uint32_t setIndex);
  void bindPipeline(const pipe::ComputePipeline &pipeline,
                    std::vector<pipe::DescriptorSet> &sets);
  void dispatch(uint32_t groupCountX, uint32_t groupCountY,
                uint32_t groupCountZ);

  void hostRead(const res::Buffer &buffer);
  void hostRead(const res::BufferSlice &slice);

  void beginProfileScope(std::string_view name);
  void endProfileScope();

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  std::optional<std::reference_wrapper<Profiler>> profiler_{};

  // components
  core::CommandBuffer command_buffer_;
  std::vector<VkDescriptorSet> bound_descriptors_{};

  // state
  bool active_{false};
  bool submitted_{false};

  void ensureRecording(std::string_view op) const;
  void ensureActive(std::string_view op) const;
  void ensureInactive(std::string_view op) const;
};

} // namespace vkr::exec
