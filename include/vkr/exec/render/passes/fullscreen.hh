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

struct FullscreenPassDesc {
  OffscreenTargetDesc target{};
  std::vector<pipeline::DescriptorBinding> descriptorBindings{};
  pipeline::DescriptorPoolDesc descriptorPool{};
  std::vector<VkClearValue> clearValues{};
  std::vector<RenderPassInputDesc> inputs{};
  pipeline::GraphicsPipelineDesc graphicsPipeline{};

  auto targetDesc(OffscreenTargetDesc desc) -> FullscreenPassDesc & {
    target = std::move(desc);
    return *this;
  }

  auto color(uint32_t width, uint32_t height, VkFormat format)
      -> FullscreenPassDesc & {
    target.colorAttachment(width, height, format);
    return *this;
  }

  auto color(ColorAttachmentDesc desc) -> FullscreenPassDesc & {
    target.colorAttachment(std::move(desc));
    return *this;
  }

  auto colorUsage(VkImageUsageFlags usage) -> FullscreenPassDesc & {
    target.color.usage = usage;
    return *this;
  }

  auto sampledColor(bool enabled = true) -> FullscreenPassDesc & {
    target.sampledColor(enabled);
    return *this;
  }

  auto colorFinalLayout(VkImageLayout layout) -> FullscreenPassDesc & {
    target.color.finalLayout = layout;
    return *this;
  }

  auto colorSampler(resource::SamplerDesc desc) -> FullscreenPassDesc & {
    target.color.withSampler(std::move(desc));
    return *this;
  }

  auto depth(VkFormat format) -> FullscreenPassDesc & {
    target.depthAttachment(target.width(), target.height(), format);
    return *this;
  }

  auto depth(uint32_t width, uint32_t height, VkFormat format)
      -> FullscreenPassDesc & {
    target.depthAttachment(width, height, format);
    return *this;
  }

  auto disableDepthAttachment() -> FullscreenPassDesc & {
    target.disableDepth();
    return *this;
  }

  auto descriptorPoolDesc(pipeline::DescriptorPoolDesc desc)
      -> FullscreenPassDesc & {
    descriptorPool = std::move(desc);
    return *this;
  }

  auto descriptors(std::vector<pipeline::DescriptorBinding> bindings)
      -> FullscreenPassDesc & {
    descriptorBindings = std::move(bindings);
    return *this;
  }

  auto clearDescriptors() noexcept -> FullscreenPassDesc & {
    descriptorBindings.clear();
    return *this;
  }

  auto inputsList(std::vector<RenderPassInputDesc> descs)
      -> FullscreenPassDesc & {
    inputs = std::move(descs);
    return *this;
  }

  auto clearInputs() noexcept -> FullscreenPassDesc & {
    inputs.clear();
    return *this;
  }

  auto clearValuesList(std::vector<VkClearValue> values)
      -> FullscreenPassDesc & {
    clearValues = std::move(values);
    return *this;
  }

  auto clearClearValues() noexcept -> FullscreenPassDesc & {
    clearValues.clear();
    return *this;
  }

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

  auto pipelineDesc(pipeline::GraphicsPipelineDesc desc)
      -> FullscreenPassDesc & {
    graphicsPipeline = std::move(desc);
    return *this;
  }

  auto pipeline(std::string name) -> FullscreenPassDesc & {
    graphicsPipeline.setName(std::move(name));
    return *this;
  }

  auto vertexInput(scene::VertexInputDesc desc) -> FullscreenPassDesc & {
    graphicsPipeline.vertexInputDesc(std::move(desc));
    return *this;
  }

  auto shader(pipeline::GraphicsShaderStageDesc shaderDesc)
      -> FullscreenPassDesc & {
    graphicsPipeline.shader(std::move(shaderDesc));
    return *this;
  }

  auto vertexShader(resource::ShaderModuleDesc shaderDesc,
                    std::string entryPoint = "main") -> FullscreenPassDesc & {
    graphicsPipeline.vertexShader(std::move(shaderDesc), std::move(entryPoint));
    return *this;
  }

  auto fragmentShader(resource::ShaderModuleDesc shaderDesc,
                      std::string entryPoint = "main") -> FullscreenPassDesc & {
    graphicsPipeline.fragmentShader(std::move(shaderDesc),
                                    std::move(entryPoint));
    return *this;
  }

  auto depthTest(VkBool32 testEnable = VK_TRUE, VkBool32 writeEnable = VK_TRUE,
                 VkCompareOp compareOp = VK_COMPARE_OP_LESS)
      -> FullscreenPassDesc & {
    graphicsPipeline.depth(testEnable, writeEnable, compareOp);
    return *this;
  }

  auto disableDepthTest() -> FullscreenPassDesc & {
    graphicsPipeline.disableDepth();
    return *this;
  }

  auto readOnlyDepthTest() -> FullscreenPassDesc & {
    graphicsPipeline.readOnlyDepth();
    return *this;
  }

  auto rasterize(pipeline::GraphicsRasterizationDesc desc)
      -> FullscreenPassDesc & {
    graphicsPipeline.rasterize(desc);
    return *this;
  }

  auto noCull() -> FullscreenPassDesc & {
    graphicsPipeline.noCull();
    return *this;
  }

  auto blend(pipeline::GraphicsColorBlendDesc desc) -> FullscreenPassDesc & {
    graphicsPipeline.blend(std::move(desc));
    return *this;
  }

  auto alphaBlend() -> FullscreenPassDesc & {
    graphicsPipeline.alphaBlend();
    return *this;
  }

  auto fullscreenPipeline(std::string name) -> FullscreenPassDesc & {
    graphicsPipeline =
        pipeline::GraphicsPipelineDesc::fullscreen(std::move(name));
    return *this;
  }

  [[nodiscard]] static auto offscreen(uint32_t width, uint32_t height,
                                      VkFormat format) -> FullscreenPassDesc {
    FullscreenPassDesc desc{};
    return desc.color(width, height, format)
        .disableDepthAttachment()
        .fullscreenPipeline("fullscreen");
  }

  [[nodiscard]] static auto sampledOffscreen(uint32_t width, uint32_t height,
                                             VkFormat format)
      -> FullscreenPassDesc {
    FullscreenPassDesc desc{};
    return desc.color(ColorAttachmentDesc::sampled2D(width, height, format))
        .disableDepthAttachment()
        .fullscreenPipeline("fullscreen");
  }

  [[nodiscard]] static auto postProcess(uint32_t width, uint32_t height,
                                        VkFormat format,
                                        std::string pipelineName)
      -> FullscreenPassDesc {
    FullscreenPassDesc desc{};
    return desc.color(ColorAttachmentDesc::sampled2D(width, height, format))
        .clearColor(0.0F, 0.0F, 0.0F, 1.0F)
        .fullscreenPipeline(std::move(pipelineName));
  }
};

class FullscreenPass : public Pass {
public:
  FullscreenPass(Executor &executor, const core::Device &device,
                 const core::CommandPool &commandPool,
                 std::vector<RenderPassSource> sources = {});
  FullscreenPass(Executor &executor, const core::Device &device,
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
  Executor &executor_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  scene::Scene *scene_{nullptr};

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
