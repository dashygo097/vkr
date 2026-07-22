#include "vkr/exec/profiler/profiler.hh"
#include "vkr/logger.hh"
#include <algorithm>
#include <limits>

namespace vkr::exec {

Profiler::Profiler(const core::Device &device,
                   const core::CommandPool &commandPool,
                   const ProfilerDesc &desc)
    : device_(device), command_pool_(commandPool), desc_(desc) {
  create();
}

Profiler::~Profiler() { destroy(); }

void Profiler::create() {
  if (!desc_.isValid()) {
    VKR_EXEC_ERROR("ProfilerDesc is invalid");
  }

  if (!desc_.enableGpuTimestamps) {
    VKR_EXEC_INFO("GPU profiler disabled by config");
    return;
  }

  const auto &families = device_.queueFamilies();
  if (command_pool_.queueFamily() >= families.size()) {
    VKR_EXEC_WARN("GPU profiler disabled: command pool queue family {} is out "
                  "of range",
                  command_pool_.queueFamily());
    return;
  }

  timestamp_valid_bits_ =
      families[command_pool_.queueFamily()].timestampValidBits;
  if (timestamp_valid_bits_ == 0) {
    VKR_EXEC_WARN("GPU profiler disabled: queue family {} does not support "
                  "timestamps",
                  command_pool_.queueFamily());
    return;
  }

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device_.physicalDevice(), &properties);
  timestamp_period_ = properties.limits.timestampPeriod;
  if (timestamp_period_ <= 0.0F) {
    VKR_EXEC_WARN("GPU profiler disabled: invalid timestamp period");
    return;
  }

  VkQueryPoolCreateInfo queryPoolInfo{};
  queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
  queryPoolInfo.queryCount = queryCount();

  if (vkCreateQueryPool(device_.device(), &queryPoolInfo, nullptr,
                        &query_pool_) != VK_SUCCESS) {
    VKR_EXEC_WARN("GPU profiler disabled: failed to create timestamp query "
                  "pool");
    query_pool_ = VK_NULL_HANDLE;
    return;
  }

  enabled_ = true;
  VKR_EXEC_INFO("GPU profiler enabled: maxScopes={}, timestampPeriod={} ns",
                desc_.maxScopes, timestamp_period_);
}

void Profiler::destroy() noexcept {
  if (query_pool_ != VK_NULL_HANDLE) {
    vkDestroyQueryPool(device_.device(), query_pool_, nullptr);
    query_pool_ = VK_NULL_HANDLE;
  }

  enabled_ = false;
  frame_active_ = false;
}

void Profiler::beginFrame(VkCommandBuffer commandBuffer) {
  if (!enabled_) {
    return;
  }

  if (commandBuffer == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("Profiler::beginFrame received null command buffer");
  }

  scopes_.clear();
  scope_stack_.clear();
  next_query_ = 0;
  frame_active_ = true;

  vkCmdResetQueryPool(commandBuffer, query_pool_, 0, queryCount());
}

void Profiler::endFrame(VkCommandBuffer) {
  if (!enabled_) {
    return;
  }

  if (!frame_active_) {
    VKR_EXEC_ERROR("Profiler::endFrame called without active frame");
  }

  if (!scope_stack_.empty()) {
    VKR_EXEC_ERROR("Profiler frame ended with {} active scope(s)",
                   scope_stack_.size());
  }

  frame_active_ = false;
}

void Profiler::beginScope(VkCommandBuffer commandBuffer,
                          std::string_view name) {
  if (!enabled_) {
    return;
  }

  if (!frame_active_) {
    VKR_EXEC_ERROR("Profiler::beginScope called without active frame");
  }

  if (commandBuffer == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("Profiler::beginScope received null command buffer");
  }

  if (next_query_ + 1 >= queryCount()) {
    VKR_EXEC_WARN("GPU profiler scope limit reached, skipping scope '{}'",
                  std::string(name));
    return;
  }

  Scope scope{};
  scope.name = name.empty() ? "unnamed" : std::string(name);
  scope.beginQuery = next_query_++;
  scope.endQuery = next_query_++;

  const size_t scopeIndex = scopes_.size();
  scopes_.push_back(std::move(scope));
  scope_stack_.push_back(scopeIndex);

  vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                      query_pool_, scopes_.back().beginQuery);
}

void Profiler::endScope(VkCommandBuffer commandBuffer) {
  if (!enabled_) {
    return;
  }

  if (!frame_active_) {
    VKR_EXEC_ERROR("Profiler::endScope called without active frame");
  }

  if (commandBuffer == VK_NULL_HANDLE) {
    VKR_EXEC_ERROR("Profiler::endScope received null command buffer");
  }

  if (scope_stack_.empty()) {
    VKR_EXEC_WARN("GPU profiler endScope called without an active scope");
    return;
  }

  const size_t scopeIndex = scope_stack_.back();
  scope_stack_.pop_back();

  vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                      query_pool_, scopes_[scopeIndex].endQuery);
}

auto Profiler::collect() -> ProfileReport {
  ProfileReport report{};
  report.gpuTimestampsEnabled = enabled_;

  if (!enabled_ || scopes_.empty()) {
    return report;
  }

  std::vector<uint64_t> timestamps(next_query_, 0);
  const VkResult result = vkGetQueryPoolResults(
      device_.device(), query_pool_, 0, next_query_,
      sizeof(uint64_t) * timestamps.size(), timestamps.data(),
      sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

  if (result != VK_SUCCESS) {
    VKR_EXEC_WARN("Failed to collect GPU profiler timestamp results");
    return report;
  }

  report.samples.reserve(scopes_.size());
  for (const auto &scope : scopes_) {
    const uint64_t delta =
        timestampDelta(timestamps[scope.beginQuery], timestamps[scope.endQuery]);
    const double nanoseconds = static_cast<double>(delta) * timestamp_period_;
    report.samples.push_back(ProfileSample{
        .name = scope.name,
        .gpuMilliseconds = nanoseconds / 1'000'000.0,
    });
  }

  return report;
}

auto Profiler::queryCount() const noexcept -> uint32_t {
  return desc_.maxScopes * 2;
}

auto Profiler::timestampDelta(uint64_t begin, uint64_t end) const
    -> uint64_t {
  if (timestamp_valid_bits_ >= 64) {
    return end >= begin ? end - begin : 0;
  }

  const uint64_t mask = (uint64_t{1} << timestamp_valid_bits_) - 1U;
  begin &= mask;
  end &= mask;

  if (end >= begin) {
    return end - begin;
  }

  return (mask - begin) + end + 1U;
}

} // namespace vkr::exec
