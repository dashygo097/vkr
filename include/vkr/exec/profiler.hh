#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkr::exec {

struct ProfilerDesc {
  bool enableGpuTimestamps{false};
  uint32_t maxScopes{64};
  uint32_t warmupFrames{0};
  uint32_t captureFrames{1};
  bool logReport{true};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return maxScopes > 0 && captureFrames > 0;
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("enableGpuTimestamps", enableGpuTimestamps);
    ar("maxScopes", maxScopes);
    ar("warmupFrames", warmupFrames);
    ar("captureFrames", captureFrames);
    ar("logReport", logReport);
  }
};

struct ProfileSample {
  std::string name{};
  double milliseconds{0.0};
  double minMilliseconds{0.0};
  double medianMilliseconds{0.0};
  double maxMilliseconds{0.0};
  uint32_t captureCount{1};
};

struct ProfileReport {
  bool gpuTimestampsEnabled{false};
  std::vector<ProfileSample> gpuSamples{};
  std::vector<ProfileSample> cpuSamples{};

  [[nodiscard]] auto empty() const noexcept -> bool {
    return gpuSamples.empty() && cpuSamples.empty();
  }
};

class Profiler {
public:
  Profiler(const core::Device &device, const core::CommandPool &commandPool,
           const ProfilerDesc &desc);
  ~Profiler();

  Profiler(const Profiler &) = delete;
  auto operator=(const Profiler &) -> Profiler & = delete;

  void beginFrame(VkCommandBuffer commandBuffer);
  void endFrame(VkCommandBuffer commandBuffer);
  void beginScope(VkCommandBuffer commandBuffer, std::string_view name,
                  VkPipelineStageFlagBits stage =
                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
  void endScope(VkCommandBuffer commandBuffer,
                VkPipelineStageFlagBits stage =
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

  [[nodiscard]] auto collect() -> ProfileReport;
  [[nodiscard]] auto enabled() const noexcept -> bool { return enabled_; }
  [[nodiscard]] auto desc() const noexcept -> const ProfilerDesc & {
    return desc_;
  }

private:
  struct Scope {
    std::string name{};
    uint32_t beginQuery{0};
    uint32_t endQuery{0};
  };

  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  // components
  ProfilerDesc desc_{};
  VkQueryPool query_pool_{VK_NULL_HANDLE};
  std::vector<Scope> scopes_{};

  // states
  std::vector<size_t> scope_stack_{};
  uint32_t next_query_{0};
  uint32_t timestamp_valid_bits_{0};
  float timestamp_period_{0.0F};
  bool enabled_{false};
  bool frame_active_{false};

  // helpers
  void create();
  void destroy() noexcept;
  [[nodiscard]] auto queryCount() const noexcept -> uint32_t;
  [[nodiscard]] auto timestampDelta(uint64_t begin, uint64_t end) const
      -> uint64_t;
};

} // namespace vkr::exec
