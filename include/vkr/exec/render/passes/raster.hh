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
#include "vkr/scene/geometry/mesh.hh"
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace vkr::exec {

struct RasterPassDesc {
  pipe::OffscreenTargetDesc target{};
  std::vector<pipe::DescriptorSetLayoutDesc> descriptorLayouts{};
  std::vector<VkClearValue> clearValues{};
  pipe::GraphicsPipelineDesc pipeline{};
  std::vector<RenderPassInputDesc> inputs{};

  auto layout(uint32_t setIndex) -> pipe::DescriptorSetLayoutDesc & {
    if (setIndex >= descriptorLayouts.size()) {
      descriptorLayouts.resize(static_cast<size_t>(setIndex) + 1);
    }
    return descriptorLayouts[setIndex];
  }

  auto uniform(uint32_t setIndex, uint32_t binding,
               VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT)
      -> RasterPassDesc & {
    layout(setIndex).uniform(binding, stages);
    return *this;
  }

  auto texture(uint32_t setIndex, uint32_t binding,
               VkShaderStageFlags stages = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> RasterPassDesc & {
    layout(setIndex).texture(binding, stages);
    return *this;
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
    desc.target = pipe::OffscreenTargetDesc::sampledColorDepth(
        width, height, colorFormat, depthFormat);
    desc.pipeline = pipe::GraphicsPipelineDesc::mesh(
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
        pipe::OffscreenTargetDesc::shadowMap(width, height, depthFormat);
    desc.pipeline = pipe::GraphicsPipelineDesc::shadowMap(
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
  RasterPass(RenderExecutor &executor, const core::Device &device);
  ~RasterPass() override;

  RasterPass(const RasterPass &) = delete;
  auto operator=(const RasterPass &) -> RasterPass & = delete;

  void create() override;
  void destroy() noexcept override;
  void update(const RasterPassDesc &desc);
  void record() override;
  auto mesh(const scene::IMesh &mesh) -> RasterPass &;

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

  [[nodiscard]] auto target() -> pipe::OffscreenTarget &;
  [[nodiscard]] auto target() const -> const pipe::OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t) -> pipe::OffscreenTarget & override {
    return target();
  }
  [[nodiscard]] auto target(uint32_t) const
      -> const pipe::OffscreenTarget & override {
    return target();
  }

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
  // dependencies
  RenderExecutor &executor_;
  const core::Device &device_;

  // components
  RasterPassDesc desc_{};
  std::unique_ptr<pipe::OffscreenTarget> target_{};
  std::unique_ptr<pipe::RenderPass> render_pass_{};
  std::unique_ptr<Framebuffers> framebuffers_{};
  pipe::DescriptorPool descriptor_pool_;
  std::vector<std::unique_ptr<pipe::DescriptorSetLayout>>
      descriptor_layouts_{};
  std::vector<std::vector<pipe::DescriptorSet>> descriptor_sets_{};
  std::unique_ptr<pipe::GraphicsPipeline> pipeline_{};
  std::vector<std::reference_wrapper<Pass>> sources_{};
  std::vector<std::reference_wrapper<const scene::IMesh>> meshes_{};

  // helpers
  void createTarget();
  void createRenderPass();
  void createFramebuffers();
  void validate(const RasterPassDesc &desc) const;
  void validateMesh(const scene::IMesh &mesh,
                    const scene::VertexInputDesc &input) const;
  void createDescriptors();
  void createPipeline();
};

} // namespace vkr::exec
