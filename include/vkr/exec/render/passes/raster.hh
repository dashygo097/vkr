#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/exec/capability.hh"
#include "vkr/exec/pass.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/exec/render/framebuffers.hh"
#include "vkr/exec/render/passes/input.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/pipeline/targets/offscreen.hh"
#include "vkr/scene/scene.hh"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace vkr::exec {

struct RasterPassDesc {
  pipeline::OffscreenTargetDesc target{};
  std::vector<pipeline::DescriptorSetLayoutDesc> descriptorLayouts{};
  std::vector<VkClearValue> clearValues{};
  pipeline::GraphicsPipelineDesc pipeline{};
  std::vector<std::string> meshNames{};
  std::vector<RenderPassInputDesc> inputs{};

  auto descriptor(uint32_t setIndex, pipeline::DescriptorBinding binding)
      -> RasterPassDesc & {
    if (setIndex >= descriptorLayouts.size()) {
      descriptorLayouts.resize(static_cast<size_t>(setIndex) + 1);
    }
    descriptorLayouts[setIndex].bindings.push_back(std::move(binding));
    return *this;
  }

  auto uniform(uint32_t setIndex, uint32_t binding,
               VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT)
      -> RasterPassDesc & {
    return descriptor(
        setIndex,
        {.layout = {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, stages}});
  }

  auto texture(uint32_t setIndex, uint32_t binding,
               VkShaderStageFlags stages = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> RasterPassDesc & {
    return descriptor(
        setIndex,
        {.layout = {binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    stages}});
  }

  auto input(uint32_t setIndex, uint32_t binding,
             VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> RasterPassDesc & {
    inputs.push_back(RenderPassInputDesc::color(setIndex, binding, stageFlags));
    return *this;
  }

  auto inputDepth(uint32_t setIndex, uint32_t binding,
                  VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> RasterPassDesc & {
    inputs.push_back(RenderPassInputDesc::depth(setIndex, binding, stageFlags));
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

  [[nodiscard]] static auto offscreen(std::string pipelineName, uint32_t width,
                                      uint32_t height, VkFormat colorFormat,
                                      VkFormat depthFormat,
                                      scene::VertexInputDesc vertexInputDesc)
      -> RasterPassDesc {
    RasterPassDesc desc{};
    desc.target = pipeline::OffscreenTargetDesc::sampledColorDepth(
        width, height, colorFormat, depthFormat);
    desc.pipeline = pipeline::GraphicsPipelineDesc::mesh(
        std::move(pipelineName), std::move(vertexInputDesc));
    return desc;
  }

  [[nodiscard]] static auto
  offscreen(std::string pipelineName, VkExtent2D extent2D, VkFormat colorFormat,
            VkFormat depthFormat, scene::VertexInputDesc vertexInputDesc)
      -> RasterPassDesc {
    return RasterPassDesc::offscreen(pipelineName, extent2D.width,
                                     extent2D.height, colorFormat, depthFormat,
                                     vertexInputDesc);
  }

  [[nodiscard]] static auto
  shadowMap(std::string pipelineName, uint32_t width, uint32_t height,
            VkFormat depthFormat, scene::VertexInputDesc vertexInputDesc,
            float depthBiasConstant = 1.25F, float depthBiasSlope = 1.75F,
            VkCullModeFlags shadowCullMode = VK_CULL_MODE_BACK_BIT,
            VkCompareOp compareOp = VK_COMPARE_OP_LESS) -> RasterPassDesc {
    RasterPassDesc desc{};
    desc.target =
        pipeline::OffscreenTargetDesc::shadowMap(width, height, depthFormat);
    desc.pipeline = pipeline::GraphicsPipelineDesc::shadowMap(
        std::move(pipelineName), std::move(vertexInputDesc), depthBiasConstant,
        depthBiasSlope, shadowCullMode, compareOp);
    desc.clearDepth();
    return desc;
  }

  [[nodiscard]] static auto
  shadowMap(std::string pipelineName, VkExtent2D extent2D, VkFormat depthFormat,
            scene::VertexInputDesc vertexInputDesc,
            float depthBiasConstant = 1.25F, float depthBiasSlope = 1.75F,
            VkCullModeFlags shadowCullMode = VK_CULL_MODE_BACK_BIT,
            VkCompareOp compareOp = VK_COMPARE_OP_LESS) -> RasterPassDesc {
    return RasterPassDesc::shadowMap(pipelineName, extent2D.width,
                                     extent2D.height, depthFormat,
                                     vertexInputDesc, depthBiasConstant,
                                     depthBiasSlope, shadowCullMode, compareOp);
  }
};

class RasterPass final : public Pass,
                         public GraphicsPipelineCapability,
                         public RenderTargetCapability {
public:
  explicit RasterPass(RenderExecutor &executor, const core::Device &device,
                      scene::Scene &scene);
  ~RasterPass() override;

  RasterPass(const RasterPass &) = delete;
  auto operator=(const RasterPass &) -> RasterPass & = delete;

  void create() override;
  void destroy() noexcept override;
  void update(const RasterPassDesc &desc);
  void record() override;

  template <typename T>
  auto uniform(uint32_t setIndex, uint32_t binding, T &buffer) -> RasterPass & {
    for (uint32_t frame = 0; frame < descriptor_sets_.size(); ++frame) {
      descriptor_sets_[frame].at(setIndex).uniform(binding, buffer, frame);
    }
    return *this;
  }

  template <typename T>
  auto texture(uint32_t setIndex, uint32_t binding, T &texture)
      -> RasterPass & {
    for (auto &frame : descriptor_sets_) {
      frame.at(setIndex).texture(binding, texture);
    }
    return *this;
  }

  template <typename T>
  auto storage(uint32_t setIndex, uint32_t binding, T &buffer) -> RasterPass & {
    for (auto &frame : descriptor_sets_) {
      frame.at(setIndex).storage(binding, buffer);
    }
    return *this;
  }

  auto addSource(Pass &source) -> RasterPass &;
  auto setSources(std::vector<std::reference_wrapper<Pass>> sources)
      -> RasterPass &;

  [[nodiscard]] auto target() -> pipeline::OffscreenTarget &;
  [[nodiscard]] auto target() const -> const pipeline::OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t) -> pipeline::OffscreenTarget & override {
    return target();
  }
  [[nodiscard]] auto target(uint32_t) const
      -> const pipeline::OffscreenTarget & override {
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
  scene::Scene &scene_;

  // components
  RasterPassDesc desc_{};
  std::unique_ptr<pipeline::OffscreenTarget> target_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::unique_ptr<Framebuffers> framebuffers_{};
  pipeline::DescriptorPool descriptor_pool_;
  std::vector<std::unique_ptr<pipeline::DescriptorSetLayout>>
      descriptor_layouts_{};
  std::vector<std::vector<pipeline::DescriptorSet>> descriptor_sets_{};
  std::unique_ptr<pipeline::GraphicsPipeline> pipeline_{};
  std::vector<std::reference_wrapper<Pass>> sources_{};

  // helpers
  void createTarget();
  void createRenderPass();
  void createFramebuffers();
  void validate(const RasterPassDesc &desc) const;
  void createDescriptors();
  void createPipeline();
};

} // namespace vkr::exec
