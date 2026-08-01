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
  std::optional<FullscreenPassInputDesc> historyInput{};
  std::vector<FullscreenPassInputDesc> inputs{};
  pipeline::GraphicsPipelineDesc graphicsPipeline{};

  auto targetDesc(OffscreenTargetDesc desc) -> FeedbackFullscreenPassDesc & {
    target.target = std::move(desc);
    return *this;
  }

  auto targetDesc(FrameHistoryTargetDesc desc) -> FeedbackFullscreenPassDesc & {
    target = std::move(desc);
    return *this;
  }

  auto color(uint32_t width, uint32_t height, VkFormat format)
      -> FeedbackFullscreenPassDesc & {
    target.target.color.width = width;
    target.target.color.height = height;
    target.target.color.format = format;
    return *this;
  }

  auto color(ColorAttachmentDesc desc) -> FeedbackFullscreenPassDesc & {
    target.target.color = std::move(desc);
    return *this;
  }

  auto colorUsage(VkImageUsageFlags usage) -> FeedbackFullscreenPassDesc & {
    target.target.color.usage = usage;
    return *this;
  }

  auto sampledColor(bool enabled = true) -> FeedbackFullscreenPassDesc & {
    if (enabled) {
      target.target.color.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
      target.target.color.createSampler = true;
      if (target.target.color.finalLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        target.target.color.finalLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
      return *this;
    }

    target.target.color.usage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
    target.target.color.createSampler = false;
    return *this;
  }

  auto colorFinalLayout(VkImageLayout layout) -> FeedbackFullscreenPassDesc & {
    target.target.color.finalLayout = layout;
    return *this;
  }

  auto colorSampler(resource::SamplerDesc desc)
      -> FeedbackFullscreenPassDesc & {
    target.target.color.sampler = std::move(desc);
    target.target.color.createSampler = true;
    return *this;
  }

  auto depth(VkFormat format) -> FeedbackFullscreenPassDesc & {
    target.target.depth = DepthAttachmentDesc{
        .width = target.target.color.width,
        .height = target.target.color.height,
        .format = format,
    };
    return *this;
  }

  auto depth(uint32_t width, uint32_t height, VkFormat format)
      -> FeedbackFullscreenPassDesc & {
    target.target.depth = DepthAttachmentDesc{
        .width = width,
        .height = height,
        .format = format,
    };
    return *this;
  }

  auto disableDepthAttachment() -> FeedbackFullscreenPassDesc & {
    target.target.depth.reset();
    return *this;
  }

  auto frameCount(uint32_t count) -> FeedbackFullscreenPassDesc & {
    target.frameCount = count;
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
    historyInput = FullscreenPassInputDesc::image(binding, stageFlags);
    return *this;
  }

  auto history(FullscreenPassInputDesc desc) -> FeedbackFullscreenPassDesc & {
    historyInput = desc;
    return *this;
  }

  auto input(uint32_t binding,
             VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    inputs.push_back(FullscreenPassInputDesc::image(binding, stageFlags));
    return *this;
  }

  auto input(FullscreenPassInputDesc desc) -> FeedbackFullscreenPassDesc & {
    inputs.push_back(desc);
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
};

class FeedbackFullscreenPass final : public Pass {
public:
  FeedbackFullscreenPass(Executor &executor, const core::Device &device,
                         const core::CommandPool &commandPool,
                         std::vector<FullscreenPassSource> sources = {});
  FeedbackFullscreenPass(Executor &executor, const core::Device &device,
                         const core::CommandPool &commandPool,
                         scene::Scene &scene,
                         std::vector<FullscreenPassSource> sources = {});
  ~FeedbackFullscreenPass() override;

  FeedbackFullscreenPass(const FeedbackFullscreenPass &) = delete;
  auto operator=(const FeedbackFullscreenPass &)
      -> FeedbackFullscreenPass & = delete;

  void create() override;
  void destroy() override;
  void update(const FeedbackFullscreenPassDesc &desc);
  void record() override;

  auto addSource(FullscreenPassSource source) -> FeedbackFullscreenPass &;
  auto setSources(std::vector<FullscreenPassSource> sources)
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
  std::vector<FullscreenPassSource> sources_{};
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

  [[nodiscard]] auto resolvedInputs() const
      -> std::vector<FullscreenPassInputDesc>;
  [[nodiscard]] auto
  descriptorPoolDesc(const std::vector<FullscreenPassInputDesc> &inputs) const
      -> pipeline::DescriptorPoolDesc;
  [[nodiscard]] auto
  createDescriptorWrites(const std::vector<FullscreenPassInputDesc> &inputs)
      -> std::vector<pipeline::DescriptorSetWriteDesc>;
};

} // namespace vkr::exec
