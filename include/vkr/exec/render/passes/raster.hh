#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/exec/render/pass.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/exec/render/attachments/frame_buffer.hh"
#include "vkr/scene/scene.hh"
#include "vkr/exec/render/targets/offscreen.hh"
#include <memory>

namespace vkr::exec {

struct RasterPassDesc {
  OffscreenTargetDesc target{};
  std::vector<pipeline::DescriptorBinding> descriptorBindings{};
  pipeline::DescriptorPoolDesc descriptorPool{};
  std::vector<VkClearValue> clearValues{};
  pipeline::GraphicsPipelineDesc pipeline{};
};

class RasterPass final : public Pass {
public:
  RasterPass(Executor &executor, const core::Device &device,
             const core::CommandPool &commandPool,
             scene::Scene &scene);
  ~RasterPass() override;

  RasterPass(const RasterPass &) = delete;
  auto operator=(const RasterPass &) -> RasterPass & = delete;

  void create() override;
  void destroy() override;
  void update(const RasterPassDesc &desc);
  void record() override;

  [[nodiscard]] auto target() -> OffscreenTarget &;
  [[nodiscard]] auto target() const -> const OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t) -> OffscreenTarget & {
    return target();
  }
  [[nodiscard]] auto target(uint32_t) const
      -> const OffscreenTarget & {
    return target();
  }

  [[nodiscard]] auto editablePipeline() noexcept -> std::optional<
      std::reference_wrapper<pipeline::GraphicsPipeline>> override {
    if (!pipeline_) {
      return std::nullopt;
    }

    return *pipeline_;
  }

  [[nodiscard]] auto editablePipeline() const noexcept -> std::optional<
      std::reference_wrapper<const pipeline::GraphicsPipeline>> override {
    if (!pipeline_) {
      return std::nullopt;
    }

    return *pipeline_;
  }

private:
  // dependencies
  Executor &executor_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  scene::Scene &scene_;

  // components
  RasterPassDesc desc_{};
  std::unique_ptr<OffscreenTarget> target_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::unique_ptr<FramebufferSet> framebuffers_{};
  std::unique_ptr<pipeline::DescriptorPool> descriptor_pool_{};
  std::unique_ptr<pipeline::DescriptorSetLayout> descriptor_layout_{};
  std::unique_ptr<pipeline::DescriptorSets> descriptor_sets_{};
  std::unique_ptr<pipeline::GraphicsPipeline> pipeline_{};
  std::unique_ptr<pipeline::GraphicsPipeline> mesh_grid_pipeline_{};
  std::unique_ptr<scene::IndexBuffer> mesh_grid_index_buffer_{};
  std::string mesh_grid_name_{};

  // helpers
  void createTarget();
  void createRenderPass();
  void createFramebuffers();
  void createDescriptors();
  void createPipeline();

  [[nodiscard]] auto createDescriptorWrites() const
      -> std::vector<pipeline::DescriptorSetWriteDesc>;
  void syncSelectedMeshGrid();
  void recordSelectedMeshGrid(const std::vector<VkDescriptorSet> &sets);
};

} // namespace vkr::exec
