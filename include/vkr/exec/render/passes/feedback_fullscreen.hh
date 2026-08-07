#pragma once

#include "vkr/exec/render/passes/fullscreen.hh"
#include "vkr/exec/render/targets/frame_history.hh"
#include <optional>
#include <string>
#include <utility>

namespace vkr::exec {

struct FeedbackFullscreenPassDesc {
  FrameHistoryTargetDesc target{};
  std::vector<pipeline::DescriptorBinding> descriptorBindings{};
  pipeline::DescriptorPoolDesc descriptorPool{};
  std::vector<VkClearValue> clearValues{};
  std::optional<RenderPassInputDesc> historyInput{};
  std::vector<RenderPassInputDesc> inputs{};
  pipeline::GraphicsPipelineDesc graphicsPipeline{};

  auto targetDesc(OffscreenTargetDesc desc) -> FeedbackFullscreenPassDesc & {
    target.targetDesc(std::move(desc));
    return *this;
  }

  auto targetDesc(FrameHistoryTargetDesc desc) -> FeedbackFullscreenPassDesc & {
    target = std::move(desc);
    return *this;
  }

  auto color(uint32_t width, uint32_t height, VkFormat format)
      -> FeedbackFullscreenPassDesc & {
    target.target.colorAttachment(width, height, format);
    return *this;
  }

  auto color(ColorAttachmentDesc desc) -> FeedbackFullscreenPassDesc & {
    target.target.colorAttachment(std::move(desc));
    return *this;
  }

  auto colorUsage(VkImageUsageFlags usage) -> FeedbackFullscreenPassDesc & {
    target.target.color.usage = usage;
    return *this;
  }

  auto sampledColor(bool enabled = true) -> FeedbackFullscreenPassDesc & {
    target.target.sampledColor(enabled);
    return *this;
  }

  auto colorFinalLayout(VkImageLayout layout) -> FeedbackFullscreenPassDesc & {
    target.target.color.finalLayout = layout;
    return *this;
  }

  auto colorSampler(resource::SamplerDesc desc)
      -> FeedbackFullscreenPassDesc & {
    target.target.color.withSampler(std::move(desc));
    return *this;
  }

  auto depth(VkFormat format) -> FeedbackFullscreenPassDesc & {
    target.target.depthAttachment(target.target.width(), target.target.height(),
                                  format);
    return *this;
  }

  auto depth(uint32_t width, uint32_t height, VkFormat format)
      -> FeedbackFullscreenPassDesc & {
    target.target.depthAttachment(width, height, format);
    return *this;
  }

  auto disableDepthAttachment() -> FeedbackFullscreenPassDesc & {
    target.target.disableDepth();
    return *this;
  }

  auto frameCount(uint32_t count) -> FeedbackFullscreenPassDesc & {
    target.frames(count);
    return *this;
  }

  auto descriptorPoolDesc(pipeline::DescriptorPoolDesc desc)
      -> FeedbackFullscreenPassDesc & {
    descriptorPool = std::move(desc);
    return *this;
  }

  auto descriptors(std::vector<pipeline::DescriptorBinding> bindings)
      -> FeedbackFullscreenPassDesc & {
    descriptorBindings = std::move(bindings);
    return *this;
  }

  auto clearDescriptors() noexcept -> FeedbackFullscreenPassDesc & {
    descriptorBindings.clear();
    return *this;
  }

  auto descriptor(pipeline::DescriptorBinding binding)
      -> FeedbackFullscreenPassDesc & {
    descriptorBindings.push_back(std::move(binding));
    return *this;
  }

  auto uniform(uint32_t binding, std::string name,
               VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
               uint32_t descriptorCount = 1) -> FeedbackFullscreenPassDesc & {
    return descriptor({.name = std::move(name),
                       .layout = {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                  descriptorCount, stageFlags}});
  }

  auto texture(uint32_t binding, std::string name,
               VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
               uint32_t descriptorCount = 1) -> FeedbackFullscreenPassDesc & {
    return descriptor(
        {.name = std::move(name),
         .layout = {binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    descriptorCount, stageFlags}});
  }

  auto history(uint32_t binding,
               VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    historyInput = RenderPassInputDesc::color(binding, stageFlags);
    return *this;
  }

  auto
  historyDepth(uint32_t binding,
               VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    historyInput = RenderPassInputDesc::depth(binding, stageFlags);
    return *this;
  }

  auto history(RenderPassInputDesc desc) -> FeedbackFullscreenPassDesc & {
    historyInput = desc;
    return *this;
  }

  auto disableHistory() noexcept -> FeedbackFullscreenPassDesc & {
    historyInput.reset();
    return *this;
  }

  auto input(uint32_t binding,
             VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    inputs.push_back(RenderPassInputDesc::color(binding, stageFlags));
    return *this;
  }

  auto inputDepth(uint32_t binding,
                  VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    inputs.push_back(RenderPassInputDesc::depth(binding, stageFlags));
    return *this;
  }

  auto input(RenderPassInputDesc desc) -> FeedbackFullscreenPassDesc & {
    inputs.push_back(desc);
    return *this;
  }

  auto inputsList(std::vector<RenderPassInputDesc> descs)
      -> FeedbackFullscreenPassDesc & {
    inputs = std::move(descs);
    return *this;
  }

  auto clearInputs() noexcept -> FeedbackFullscreenPassDesc & {
    inputs.clear();
    return *this;
  }

  auto clearColor(float r, float g, float b, float a)
      -> FeedbackFullscreenPassDesc & {
    clearValues.push_back(VkClearValue{.color = {{r, g, b, a}}});
    return *this;
  }

  auto clearDepth(float depthValue = 1.0f, uint32_t stencil = 0)
      -> FeedbackFullscreenPassDesc & {
    clearValues.push_back(VkClearValue{.depthStencil = {depthValue, stencil}});
    return *this;
  }

  auto clearValuesList(std::vector<VkClearValue> values)
      -> FeedbackFullscreenPassDesc & {
    clearValues = std::move(values);
    return *this;
  }

  auto clearClearValues() noexcept -> FeedbackFullscreenPassDesc & {
    clearValues.clear();
    return *this;
  }

  auto pipelineDesc(pipeline::GraphicsPipelineDesc desc)
      -> FeedbackFullscreenPassDesc & {
    graphicsPipeline = std::move(desc);
    return *this;
  }

  auto pipeline(std::string name) -> FeedbackFullscreenPassDesc & {
    graphicsPipeline.setName(std::move(name));
    return *this;
  }

  auto vertexInput(scene::VertexInputDesc desc)
      -> FeedbackFullscreenPassDesc & {
    graphicsPipeline.vertexInputDesc(std::move(desc));
    return *this;
  }

  auto shader(pipeline::GraphicsShaderStageDesc shaderDesc)
      -> FeedbackFullscreenPassDesc & {
    graphicsPipeline.shader(std::move(shaderDesc));
    return *this;
  }

  auto vertexShader(resource::ShaderModuleDesc shaderDesc,
                    std::string entryPoint = "main")
      -> FeedbackFullscreenPassDesc & {
    graphicsPipeline.vertexShader(std::move(shaderDesc), std::move(entryPoint));
    return *this;
  }

  auto fragmentShader(resource::ShaderModuleDesc shaderDesc,
                      std::string entryPoint = "main")
      -> FeedbackFullscreenPassDesc & {
    graphicsPipeline.fragmentShader(std::move(shaderDesc),
                                    std::move(entryPoint));
    return *this;
  }

  auto depthTest(VkBool32 testEnable = VK_TRUE, VkBool32 writeEnable = VK_TRUE,
                 VkCompareOp compareOp = VK_COMPARE_OP_LESS)
      -> FeedbackFullscreenPassDesc & {
    graphicsPipeline.depth(testEnable, writeEnable, compareOp);
    return *this;
  }

  auto disableDepthTest() -> FeedbackFullscreenPassDesc & {
    graphicsPipeline.disableDepth();
    return *this;
  }

  auto readOnlyDepthTest() -> FeedbackFullscreenPassDesc & {
    graphicsPipeline.readOnlyDepth();
    return *this;
  }

  auto rasterize(pipeline::GraphicsRasterizationDesc desc)
      -> FeedbackFullscreenPassDesc & {
    graphicsPipeline.rasterize(desc);
    return *this;
  }

  auto noCull() -> FeedbackFullscreenPassDesc & {
    graphicsPipeline.noCull();
    return *this;
  }

  auto blend(pipeline::GraphicsColorBlendDesc desc)
      -> FeedbackFullscreenPassDesc & {
    graphicsPipeline.blend(std::move(desc));
    return *this;
  }

  auto alphaBlend() -> FeedbackFullscreenPassDesc & {
    graphicsPipeline.alphaBlend();
    return *this;
  }

  auto fullscreenPipeline(std::string name) -> FeedbackFullscreenPassDesc & {
    graphicsPipeline =
        pipeline::GraphicsPipelineDesc::fullscreen(std::move(name));
    return *this;
  }

  [[nodiscard]] static auto feedback(uint32_t width, uint32_t height,
                                     VkFormat format, std::string pipelineName)
      -> FeedbackFullscreenPassDesc {
    FeedbackFullscreenPassDesc desc{};
    return desc
        .targetDesc(
            OffscreenTargetDesc::sampledColorOnly(width, height, format))
        .clearColor(0.0F, 0.0F, 0.0F, 1.0F)
        .fullscreenPipeline(std::move(pipelineName));
  }
};

class FeedbackFullscreenPass final : public Pass {
public:
  FeedbackFullscreenPass(Executor &executor, const core::Device &device,
                         const core::CommandPool &commandPool,
                         std::vector<RenderPassSource> sources = {});
  FeedbackFullscreenPass(Executor &executor, const core::Device &device,
                         const core::CommandPool &commandPool,
                         scene::Scene &scene,
                         std::vector<RenderPassSource> sources = {});
  ~FeedbackFullscreenPass() override;

  FeedbackFullscreenPass(const FeedbackFullscreenPass &) = delete;
  auto operator=(const FeedbackFullscreenPass &)
      -> FeedbackFullscreenPass & = delete;

  void create() override;
  void destroy() override;
  void update(const FeedbackFullscreenPassDesc &desc);
  void record() override;

  auto addSource(RenderPassSource source) -> FeedbackFullscreenPass &;
  auto setSources(std::vector<RenderPassSource> sources)
      -> FeedbackFullscreenPass &;

  [[nodiscard]] auto target() -> OffscreenTarget &;
  [[nodiscard]] auto target() const -> const OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t frameIndex) -> OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t frameIndex) const
      -> const OffscreenTarget &;

  [[nodiscard]] auto historyTarget() -> OffscreenTarget &;
  [[nodiscard]] auto historyTarget() const -> const OffscreenTarget &;
  [[nodiscard]] auto historyTarget(uint32_t frameIndex) -> OffscreenTarget &;
  [[nodiscard]] auto historyTarget(uint32_t frameIndex) const
      -> const OffscreenTarget &;

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
  FeedbackFullscreenPassDesc desc_{};
  std::vector<RenderPassSource> sources_{};
  std::unique_ptr<FrameHistoryTarget> target_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::vector<std::unique_ptr<FramebufferSet>> framebuffers_{};
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
