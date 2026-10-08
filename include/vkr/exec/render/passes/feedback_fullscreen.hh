#pragma once

#include "vkr/exec/capability.hh"
#include "vkr/exec/render/passes/fullscreen.hh"
#include "vkr/pipeline/targets/frame_history.hh"
#include <optional>
#include <string>
#include <utility>

namespace vkr::exec {

struct FeedbackFullscreenPassDesc {
  pipeline::FrameHistoryTargetDesc target{};
  std::vector<pipeline::DescriptorBinding> descriptorBindings{};
  std::vector<VkClearValue> clearValues{};
  std::optional<RenderPassInputDesc> historyInput{};
  std::vector<RenderPassInputDesc> inputs{};
  pipeline::GraphicsPipelineDesc pipeline{};

  auto descriptor(pipeline::DescriptorBinding binding)
      -> FeedbackFullscreenPassDesc & {
    descriptorBindings.push_back(std::move(binding));
    return *this;
  }

  auto uniform(uint32_t binding,
               VkShaderStageFlags stages = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    return descriptor(
        {.layout = {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, stages}});
  }

  auto texture(uint32_t binding,
               VkShaderStageFlags stages = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    return descriptor(
        {.layout = {binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    stages}});
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
        pipeline::OffscreenTargetDesc::sampledColorOnly(width, height, format);
    desc.pipeline =
        pipeline::GraphicsPipelineDesc::fullscreen(std::move(pipelineName));
    desc.clearColor(0.0F, 0.0F, 0.0F, 1.0F);
    return desc;
  }
};

class FeedbackFullscreenPass final : public Pass,
                                     public GraphicsPipelineCapability,
                                     public RenderTargetCapability {
public:
  explicit FeedbackFullscreenPass(
      RenderExecutor &executor, const core::Device &device,
      std::vector<std::reference_wrapper<Pass>> sources = {});
  ~FeedbackFullscreenPass() override;

  FeedbackFullscreenPass(const FeedbackFullscreenPass &) = delete;
  auto operator=(const FeedbackFullscreenPass &)
      -> FeedbackFullscreenPass & = delete;

  void create() override;
  void destroy() noexcept override;
  void update(const FeedbackFullscreenPassDesc &desc);
  void record() override;

  template <typename T>
  auto uniform(uint32_t binding, T &buffer) -> FeedbackFullscreenPass & {
    descriptor_sets_.uniform(binding, buffer);
    return *this;
  }

  template <typename T>
  auto texture(uint32_t binding, T &texture) -> FeedbackFullscreenPass & {
    descriptor_sets_.texture(binding, texture);
    return *this;
  }

  template <typename T>
  auto storage(uint32_t binding, T &buffer) -> FeedbackFullscreenPass & {
    descriptor_sets_.storage(binding, buffer);
    return *this;
  }

  auto addSource(Pass &source) -> FeedbackFullscreenPass &;
  auto setSources(std::vector<std::reference_wrapper<Pass>> sources)
      -> FeedbackFullscreenPass &;

  [[nodiscard]] auto target() -> pipeline::OffscreenTarget &;
  [[nodiscard]] auto target() const -> const pipeline::OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t frameIndex)
      -> pipeline::OffscreenTarget & override;
  [[nodiscard]] auto target(uint32_t frameIndex) const
      -> const pipeline::OffscreenTarget & override;

  [[nodiscard]] auto historyTarget() -> pipeline::OffscreenTarget &;
  [[nodiscard]] auto historyTarget() const -> const pipeline::OffscreenTarget &;
  [[nodiscard]] auto historyTarget(uint32_t frameIndex)
      -> pipeline::OffscreenTarget &;
  [[nodiscard]] auto historyTarget(uint32_t frameIndex) const
      -> const pipeline::OffscreenTarget &;

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

  // components
  FeedbackFullscreenPassDesc desc_{};
  std::vector<std::reference_wrapper<Pass>> sources_{};
  std::unique_ptr<pipeline::FrameHistoryTarget> target_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::vector<std::unique_ptr<Framebuffers>> framebuffers_{};
  pipeline::DescriptorPool descriptor_pool_;
  pipeline::DescriptorSetLayout descriptor_layout_;
  pipeline::DescriptorSets descriptor_sets_;
  std::unique_ptr<pipeline::GraphicsPipeline> pipeline_{};

  // helpers
  void createTarget();
  void createRenderPass();
  void createFramebuffers();
  void validate(const FeedbackFullscreenPassDesc &desc) const;
  void createDescriptors();
  void createPipeline();

  [[nodiscard]] auto resolvedInputs() const -> std::vector<RenderPassInputDesc>;
  [[nodiscard]] auto
  createDescriptorWrites(const std::vector<RenderPassInputDesc> &inputs)
      -> std::vector<pipeline::DescriptorSetWrite>;
};

} // namespace vkr::exec
