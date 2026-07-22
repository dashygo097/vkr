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
  bool enableGpuTimestamps{true};
  uint32_t maxScopes{64};

  [[nodiscard]] auto isValid() const noexcept -> bool { return maxScopes > 0; }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("enableGpuTimestamps", enableGpuTimestamps);
    ar("maxScopes", maxScopes);
  }
};

struct ProfileSample {
  std::string name{};
  double gpuMilliseconds{0.0};
};

struct ProfileReport {
  bool gpuTimestampsEnabled{false};
  std::vector<ProfileSample> samples{};

  [[nodiscard]] auto empty() const noexcept -> bool { return samples.empty(); }
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
  void beginScope(VkCommandBuffer commandBuffer, std::string_view name);
  void endScope(VkCommandBuffer commandBuffer);

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
