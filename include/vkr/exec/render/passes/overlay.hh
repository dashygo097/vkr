#pragma once

#include "vkr/exec/capability.hh"
#include "vkr/exec/pass.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/exec/render/frame_buffer_set.hh"
#include "vkr/exec/render/targets/offscreen.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/scene/scene.hh"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vkr::exec {

struct OverlayPassDesc {
  std::vector<pipeline::DescriptorBinding> descriptorBindings{};
  pipeline::DescriptorPoolDesc descriptorPool{};
  pipeline::GraphicsPipelineDesc graphicsPipeline{};
  std::vector<std::string> meshNames{};

  [[nodiscard]] static auto
  wireframe(pipeline::GraphicsPipelineDesc pipelineDesc) -> OverlayPassDesc;
};

class OverlayPass final : public Pass,
                          public GraphicsPipelineCapability,
                          public RenderTargetCapability {
public:
  OverlayPass(RenderExecutor &executor, const core::Device &device,
              const core::CommandPool &commandPool, scene::Scene &scene,
              Pass &source);
  ~OverlayPass() override;

  OverlayPass(const OverlayPass &) = delete;
  auto operator=(const OverlayPass &) -> OverlayPass & = delete;

  void create() override;
  void destroy() noexcept override;
  void update(const OverlayPassDesc &desc);
  void record() override;

  void selectMesh(const std::string &name) noexcept;

  [[nodiscard]] auto target(uint32_t frameIndex) -> OffscreenTarget & override;
  [[nodiscard]] auto target(uint32_t frameIndex) const
      -> const OffscreenTarget & override;

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
  struct MeshEntry {
    std::reference_wrapper<const scene::IVertexBuffer> vertices;
    std::unique_ptr<scene::IndexBuffer> indices;
  };

  // dependencies
  RenderExecutor &executor_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  scene::Scene &scene_;
  Pass &source_;

  // components
  OverlayPassDesc desc_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::unique_ptr<FramebufferSet> framebuffers_{};
  std::unique_ptr<pipeline::DescriptorPool> descriptor_pool_{};
  std::unique_ptr<pipeline::DescriptorSetLayout> descriptor_layout_{};
  std::unique_ptr<pipeline::DescriptorSets> descriptor_sets_{};
  std::unique_ptr<pipeline::GraphicsPipeline> pipeline_{};
  std::unordered_map<std::string, MeshEntry> meshes_{};

  // state
  std::optional<std::reference_wrapper<RenderTargetCapability>>
      target_source_{};
  std::optional<std::reference_wrapper<const MeshEntry>> selected_mesh_{};

  void createRenderPass();
  void createFramebuffers();
  void createDescriptors();
  void createPipeline();
  void createMeshes();
};

} // namespace vkr::exec
