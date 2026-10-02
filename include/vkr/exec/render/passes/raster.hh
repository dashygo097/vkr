#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/exec/pass.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/exec/render/frame_buffer_set.hh"
#include "vkr/exec/render/passes/input.hh"
#include "vkr/exec/render/passes/source.hh"
#include "vkr/exec/render/targets/offscreen.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/scene/scene.hh"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace vkr::exec {

struct RasterPassDesc {
  OffscreenTargetDesc target{};
  std::vector<pipeline::DescriptorBinding> descriptorBindings{};
  pipeline::DescriptorPoolDesc descriptorPool{};
  std::vector<VkClearValue> clearValues{};
  pipeline::GraphicsPipelineDesc graphicsPipeline{};
  std::vector<std::string> meshNames{};
  std::vector<RenderPassInputDesc> inputs{};

  auto descriptor(pipeline::DescriptorBinding binding) -> RasterPassDesc & {
    descriptorBindings.push_back(std::move(binding));
    return *this;
  }

  auto uniform(uint32_t binding, std::string name,
               VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
               uint32_t descriptorCount = 1) -> RasterPassDesc & {
    return descriptor({.name = std::move(name),
                       .layout = {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                  descriptorCount, stageFlags}});
  }

  auto texture(uint32_t binding, std::string name,
               VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
               uint32_t descriptorCount = 1) -> RasterPassDesc & {
    return descriptor(
        {.name = std::move(name),
         .layout = {binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    descriptorCount, stageFlags}});
  }

  auto cubemap(uint32_t binding, std::string name,
               VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
               uint32_t descriptorCount = 1) -> RasterPassDesc & {
    return texture(binding, std::move(name), stageFlags, descriptorCount);
  }

  auto input(uint32_t binding,
             VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> RasterPassDesc & {
    inputs.push_back(RenderPassInputDesc::color(binding, stageFlags));
    return *this;
  }

  auto inputDepth(uint32_t binding,
                  VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> RasterPassDesc & {
    inputs.push_back(RenderPassInputDesc::depth(binding, stageFlags));
    return *this;
  }

  auto input(RenderPassInputDesc desc) -> RasterPassDesc & {
    inputs.push_back(desc);
    return *this;
  }

  auto mesh(std::string name) -> RasterPassDesc & {
    meshNames.push_back(std::move(name));
    return *this;
  }

  auto clearColor(float r, float g, float b, float a) -> RasterPassDesc & {
    clearValues.push_back(VkClearValue{.color = {{r, g, b, a}}});
    return *this;
  }

  auto clearDepth(float depthValue = 1.0f, uint32_t stencil = 0)
      -> RasterPassDesc & {
    clearValues.push_back(VkClearValue{.depthStencil = {depthValue, stencil}});
    return *this;
  }

  [[nodiscard]] static auto
  offscreen(uint32_t width, uint32_t height, VkFormat colorFormat,
            VkFormat depthFormat, std::string pipelineName,
            scene::VertexInputDesc vertexInputDesc) -> RasterPassDesc {
    RasterPassDesc desc{};
    desc.target = OffscreenTargetDesc::sampledColorDepth(
        width, height, colorFormat, depthFormat);
    desc.graphicsPipeline = pipeline::GraphicsPipelineDesc::mesh(
        std::move(pipelineName), std::move(vertexInputDesc));
    return desc;
  }

  [[nodiscard]] static auto
  shadowMap(uint32_t width, uint32_t height, VkFormat depthFormat,
            std::string pipelineName, scene::VertexInputDesc vertexInputDesc,
            float depthBiasConstant = 1.25F, float depthBiasSlope = 1.75F,
            VkCullModeFlags shadowCullMode = VK_CULL_MODE_BACK_BIT,
            VkCompareOp compareOp = VK_COMPARE_OP_LESS) -> RasterPassDesc {
    RasterPassDesc desc{};
    desc.target = OffscreenTargetDesc::shadowMap(width, height, depthFormat);
    desc.graphicsPipeline = pipeline::GraphicsPipelineDesc::shadowMap(
        std::move(pipelineName), std::move(vertexInputDesc), depthBiasConstant,
        depthBiasSlope, shadowCullMode, compareOp);
    desc.clearDepth();
    return desc;
  }
};

class RasterPass final : public Pass, public GraphicsPipelineCapability {
public:
  RasterPass(RenderExecutor &executor, const core::Device &device,
             const core::CommandPool &commandPool, scene::Scene &scene);
  ~RasterPass() override;

  RasterPass(const RasterPass &) = delete;
  auto operator=(const RasterPass &) -> RasterPass & = delete;

  void create() override;
  void destroy() override;
  void update(const RasterPassDesc &desc);
  void record() override;
  auto addSource(RenderPassSource source) -> RasterPass &;
  auto setSources(std::vector<RenderPassSource> sources) -> RasterPass &;

  [[nodiscard]] auto target() -> OffscreenTarget &;
  [[nodiscard]] auto target() const -> const OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t) -> OffscreenTarget & { return target(); }
  [[nodiscard]] auto target(uint32_t) const -> const OffscreenTarget & {
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
  RenderExecutor &executor_;
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
  std::vector<RenderPassSource> sources_{};

  // helpers
  void createTarget();
  void createRenderPass();
  void createFramebuffers();
  void createDescriptors();
  void createPipeline();

  [[nodiscard]] auto createDescriptorWrites() const
      -> std::vector<pipeline::DescriptorSetWriteDesc>;
  [[nodiscard]] auto descriptorPoolDesc() const -> pipeline::DescriptorPoolDesc;
  void syncSelectedMeshGrid();
  void recordSelectedMeshGrid(const pipeline::DescriptorSets &sets);
};

} // namespace vkr::exec
