#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/core/sync/fence.hh"

namespace vkr::exec {

class ComputeExecutor {
public:
  explicit ComputeExecutor(const core::Device &device,
                           const core::CommandPool &commandPool);
  ~ComputeExecutor();

  ComputeExecutor(const ComputeExecutor &) = delete;
  auto operator=(const ComputeExecutor &) -> ComputeExecutor & = delete;

  void begin();
  void submitAndWait();
  void end();

  [[nodiscard]] auto commandBuffer() const -> VkCommandBuffer;

  void bindComputePipeline(
      VkPipeline pipeline, VkPipelineLayout pipelineLayout,
      const std::vector<VkDescriptorSet> &descriptorSets);
  void dispatch(uint32_t groupCountX, uint32_t groupCountY,
                uint32_t groupCountZ);

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  VkCommandBuffer command_buffer_{VK_NULL_HANDLE};

  // state
  bool active_{false};
  bool submitted_{false};

  void allocateCommandBuffer();
  void freeCommandBuffer() noexcept;
  void ensureActive(const char *op) const;
  void ensureInactive(const char *op) const;
};

} // namespace vkr::exec
