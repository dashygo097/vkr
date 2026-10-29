#pragma once

#include "vkr/exec/capability.hh"
#include "vkr/exec/pass.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/exec/render/framebuffers.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/pipeline/targets/offscreen.hh"
#include "vkr/scene/geometry/mesh.hh"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vkr::res {
class UniformBuffer;
} // namespace vkr::res

namespace vkr::exec {

struct OverlayPassDesc {
  std::vector<pipe::DescriptorSetLayoutDesc> descriptorLayouts{};
  pipe::GraphicsPipelineDesc pipeline{};

  auto layout(uint32_t setIndex) -> pipe::DescriptorSetLayoutDesc & {
    if (setIndex >= descriptorLayouts.size()) {
      descriptorLayouts.resize(static_cast<size_t>(setIndex) + 1);
    }
    return descriptorLayouts[setIndex];
  }

  auto uniform(uint32_t setIndex, uint32_t binding,
               VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT)
      -> OverlayPassDesc & {
    layout(setIndex).uniform(binding, stages);
    return *this;
  }

  auto texture(uint32_t setIndex, uint32_t binding,
               VkShaderStageFlags stages = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> OverlayPassDesc & {
    layout(setIndex).texture(binding, stages);
    return *this;
  }

  [[nodiscard]] static auto wireframe(pipe::GraphicsPipelineDesc pipelineDesc)
      -> OverlayPassDesc;
};

class OverlayPass final : public Pass,
                          public GraphicsPipelineCapability,
                          public RenderTargetCapability {
public:
  explicit OverlayPass(RenderExecutor &executor, const core::Device &device,
                       const core::CommandPool &commandPool, Pass &source);
  ~OverlayPass() override;

  OverlayPass(const OverlayPass &) = delete;
  auto operator=(const OverlayPass &) -> OverlayPass & = delete;

  void create() override;
  void destroy() noexcept override;
  void update(const OverlayPassDesc &desc);
  void record() override;
  [[nodiscard]] auto dependencies() const
      -> std::vector<std::reference_wrapper<const Pass>> override;
  auto mesh(std::string name, const scene::IMesh &mesh) -> OverlayPass &;

  auto uniform(uint32_t setIndex, uint32_t binding, const res::Buffer &buffer)
      -> OverlayPass &;
  auto uniform(uint32_t setIndex, uint32_t binding,
               const res::BufferSlice &slice) -> OverlayPass &;
  auto uniform(uint32_t setIndex, uint32_t binding,
               const std::vector<res::UniformBuffer> &buffers) -> OverlayPass &;

  auto texture(uint32_t setIndex, uint32_t binding, const res::Texture &texture)
      -> OverlayPass &;
  auto texture(uint32_t setIndex, uint32_t binding, const res::Cubemap &cubemap)
      -> OverlayPass &;
  auto texture(uint32_t setIndex, uint32_t binding,
               const pipe::ColorAttachment &attachment) -> OverlayPass &;
  auto texture(uint32_t setIndex, uint32_t binding,
               const pipe::DepthAttachment &attachment) -> OverlayPass &;

  auto storage(uint32_t setIndex, uint32_t binding, const res::Buffer &buffer)
      -> OverlayPass &;
  auto storage(uint32_t setIndex, uint32_t binding,
               const res::BufferSlice &slice) -> OverlayPass &;

  void selectMesh(const std::string &name) noexcept;

  [[nodiscard]] auto target(uint32_t frameIndex)
      -> pipe::OffscreenTarget & override;
  [[nodiscard]] auto target(uint32_t frameIndex) const
      -> const pipe::OffscreenTarget & override;

  [[nodiscard]] auto editablePipeline() noexcept -> std::optional<
      std::reference_wrapper<pipe::GraphicsPipeline>> override {
    if (!pipeline_) {
      return std::nullopt;
    }
    return *pipeline_;
  }

  [[nodiscard]] auto editablePipeline() const noexcept -> std::optional<
      std::reference_wrapper<const pipe::GraphicsPipeline>> override {
    if (!pipeline_) {
      return std::nullopt;
    }
    return *pipeline_;
  }

private:
  struct MeshEntry {
    std::reference_wrapper<const scene::IMesh> source;
    std::unique_ptr<scene::IndexBuffer> indices;
  };

  // dependencies
  RenderExecutor &executor_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  Pass &source_;

  // components
  OverlayPassDesc desc_{};
  std::unique_ptr<pipe::RenderPass> render_pass_{};
  std::unique_ptr<Framebuffers> framebuffers_{};
  pipe::DescriptorPool descriptor_pool_;
  std::vector<std::unique_ptr<pipe::DescriptorSetLayout>> descriptor_layouts_{};
  std::vector<std::vector<pipe::DescriptorSet>> descriptor_sets_{};
  std::unique_ptr<pipe::GraphicsPipeline> pipeline_{};
  std::unordered_map<std::string, MeshEntry> meshes_{};

  // state
  std::optional<std::reference_wrapper<RenderTargetCapability>>
      target_source_{};
  std::optional<std::reference_wrapper<const MeshEntry>> selected_mesh_{};

  void createRenderPass();
  void createFramebuffers();
  void validate(const OverlayPassDesc &desc) const;
  void createDescriptors();
  void createPipeline();
  void createMeshes();
  void validateMesh(const scene::IMesh &mesh,
                    const scene::VertexInputDesc &input) const;
  [[nodiscard]] auto createIndices(const scene::IMesh &mesh)
      -> std::unique_ptr<scene::IndexBuffer>;
};

} // namespace vkr::exec
