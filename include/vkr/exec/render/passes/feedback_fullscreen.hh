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

  [[nodiscard]] static auto feedback(uint32_t width, uint32_t height,
                                     VkFormat format, std::string pipelineName)
      -> FeedbackFullscreenPassDesc {
    FeedbackFullscreenPassDesc desc{};
    desc.target.target =
        OffscreenTargetDesc::sampledColorOnly(width, height, format);
    desc.graphicsPipeline =
        pipeline::GraphicsPipelineDesc::fullscreen(std::move(pipelineName));
    desc.clearColor(0.0F, 0.0F, 0.0F, 1.0F);
    return desc;
  }
};

class FeedbackFullscreenPass final : public Pass,
                                     public GraphicsPipelineCapability {
public:
  FeedbackFullscreenPass(RenderExecutor &executor, const core::Device &device,
                         const core::CommandPool &commandPool,
                         std::vector<RenderPassSource> sources = {});
  FeedbackFullscreenPass(RenderExecutor &executor, const core::Device &device,
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
  RenderExecutor &executor_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  std::optional<std::reference_wrapper<scene::Scene>> scene_{};

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
