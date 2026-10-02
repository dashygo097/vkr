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
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace vkr::exec {

struct FullscreenPassDesc {
  OffscreenTargetDesc target{};
  std::vector<pipeline::DescriptorBinding> descriptorBindings{};
  pipeline::DescriptorPoolDesc descriptorPool{};
  std::vector<VkClearValue> clearValues{};
  std::vector<RenderPassInputDesc> inputs{};
  pipeline::GraphicsPipelineDesc graphicsPipeline{};

  auto descriptor(pipeline::DescriptorBinding binding) -> FullscreenPassDesc & {
    descriptorBindings.push_back(std::move(binding));
    return *this;
  }

  auto uniform(uint32_t binding, std::string name,
               VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
               uint32_t descriptorCount = 1) -> FullscreenPassDesc & {
    return descriptor({.name = std::move(name),
                       .layout = {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                  descriptorCount, stageFlags}});
  }

  auto texture(uint32_t binding, std::string name,
               VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
               uint32_t descriptorCount = 1) -> FullscreenPassDesc & {
    return descriptor(
        {.name = std::move(name),
         .layout = {binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    descriptorCount, stageFlags}});
  }

  auto input(uint32_t binding,
             VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FullscreenPassDesc & {
    inputs.push_back(RenderPassInputDesc::color(binding, stageFlags));
    return *this;
  }

  auto inputDepth(uint32_t binding,
                  VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FullscreenPassDesc & {
    inputs.push_back(RenderPassInputDesc::depth(binding, stageFlags));
    return *this;
  }

  auto input(RenderPassInputDesc desc) -> FullscreenPassDesc & {
    inputs.push_back(desc);
    return *this;
  }

  auto clearColor(float r, float g, float b, float a) -> FullscreenPassDesc & {
    clearValues.push_back(VkClearValue{.color = {{r, g, b, a}}});
    return *this;
  }

  auto clearDepth(float depthValue = 1.0f, uint32_t stencil = 0)
      -> FullscreenPassDesc & {
    clearValues.push_back(VkClearValue{.depthStencil = {depthValue, stencil}});
    return *this;
  }

  [[nodiscard]] static auto offscreen(uint32_t width, uint32_t height,
                                      VkFormat format) -> FullscreenPassDesc {
    FullscreenPassDesc desc{};
    desc.target = OffscreenTargetDesc::colorOnly(width, height, format);
    desc.graphicsPipeline =
        pipeline::GraphicsPipelineDesc::fullscreen("fullscreen");
    return desc;
  }

  [[nodiscard]] static auto sampledOffscreen(uint32_t width, uint32_t height,
                                             VkFormat format)
      -> FullscreenPassDesc {
    FullscreenPassDesc desc{};
    desc.target =
        OffscreenTargetDesc::sampledColorOnly(width, height, format);
    desc.graphicsPipeline =
        pipeline::GraphicsPipelineDesc::fullscreen("fullscreen");
    return desc;
  }

  [[nodiscard]] static auto postProcess(uint32_t width, uint32_t height,
                                        VkFormat format,
                                        std::string pipelineName)
      -> FullscreenPassDesc {
    FullscreenPassDesc desc{};
    desc.target =
        OffscreenTargetDesc::sampledColorOnly(width, height, format);
    desc.graphicsPipeline =
        pipeline::GraphicsPipelineDesc::fullscreen(std::move(pipelineName));
    desc.clearColor(0.0F, 0.0F, 0.0F, 1.0F);
    return desc;
  }
};

class FullscreenPass : public Pass, public GraphicsPipelineCapability {
public:
  FullscreenPass(RenderExecutor &executor, const core::Device &device,
                 const core::CommandPool &commandPool,
                 std::vector<RenderPassSource> sources = {});
  FullscreenPass(RenderExecutor &executor, const core::Device &device,
                 const core::CommandPool &commandPool, scene::Scene &scene,
                 std::vector<RenderPassSource> sources = {});
  ~FullscreenPass() override;

  FullscreenPass(const FullscreenPass &) = delete;
  auto operator=(const FullscreenPass &) -> FullscreenPass & = delete;

  void create() override;
  void destroy() override;
  void update(const FullscreenPassDesc &desc);
  void record() override;

  auto addSource(RenderPassSource source) -> FullscreenPass &;
  auto setSources(std::vector<RenderPassSource> sources) -> FullscreenPass &;

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
  std::optional<std::reference_wrapper<scene::Scene>> scene_{};

  // components
  FullscreenPassDesc desc_{};
  std::vector<RenderPassSource> sources_{};
  std::unique_ptr<OffscreenTarget> target_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::unique_ptr<FramebufferSet> framebuffers_{};
  std::unique_ptr<pipeline::DescriptorPool> descriptor_pool_{};
  std::unique_ptr<pipeline::DescriptorSetLayout> descriptor_layout_{};
  std::unique_ptr<pipeline::DescriptorSets> descriptor_sets_{};
  std::unique_ptr<pipeline::GraphicsPipeline> pipeline_{};

  // helpers
  void createTarget();
  void createRenderPass();
  void createFramebuffers();
  void createDescriptors();
  void createPipeline();

  [[nodiscard]] auto resolvedInputs() const -> std::vector<RenderPassInputDesc>;
  [[nodiscard]] auto
  descriptorPoolDesc(const std::vector<RenderPassInputDesc> &inputs) const
      -> pipeline::DescriptorPoolDesc;
  [[nodiscard]] auto
  createDescriptorWrites(const std::vector<RenderPassInputDesc> &inputs)
      -> std::vector<pipeline::DescriptorSetWriteDesc>;
};

} // namespace vkr::exec
