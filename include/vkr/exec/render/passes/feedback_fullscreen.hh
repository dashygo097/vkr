#pragma once

#include "vkr/exec/capability.hh"
#include "vkr/exec/render/passes/fullscreen.hh"
#include "vkr/pipeline/targets/frame_history.hh"
#include <optional>
#include <string>
#include <utility>

namespace vkr::exec {

struct FeedbackFullscreenPassDesc {
  pipe::FrameHistoryTargetDesc target{};
  std::vector<pipe::DescriptorSetLayoutDesc> descriptorLayouts{};
  std::vector<VkClearValue> clearValues{};
  std::optional<RenderPassInputDesc> historyInput{};
  std::vector<RenderPassInputDesc> inputs{};
  pipe::GraphicsPipelineDesc pipeline{};

  auto layout(uint32_t setIndex) -> pipe::DescriptorSetLayoutDesc & {
    if (setIndex >= descriptorLayouts.size()) {
      descriptorLayouts.resize(static_cast<size_t>(setIndex) + 1);
    }
    return descriptorLayouts[setIndex];
  }

  auto uniform(uint32_t setIndex, uint32_t binding,
               VkShaderStageFlags stages = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    layout(setIndex).uniform(binding, stages);
    return *this;
  }

  auto texture(uint32_t setIndex, uint32_t binding,
               VkShaderStageFlags stages = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    layout(setIndex).texture(binding, stages);
    return *this;
  }

  auto history(uint32_t setIndex, uint32_t binding,
               VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    historyInput = RenderPassInputDesc::color(setIndex, binding, stageFlags);
    return *this;
  }

  auto historyDepth(
      uint32_t setIndex, uint32_t binding,
      VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    historyInput = RenderPassInputDesc::depth(setIndex, binding, stageFlags);
    return *this;
  }

  auto history(RenderPassInputDesc desc) -> FeedbackFullscreenPassDesc & {
    historyInput = desc;
    return *this;
  }

  auto input(uint32_t setIndex, uint32_t binding,
             VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    inputs.push_back(RenderPassInputDesc::color(setIndex, binding, stageFlags));
    return *this;
  }

  auto inputDepth(uint32_t setIndex, uint32_t binding,
                  VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FeedbackFullscreenPassDesc & {
    inputs.push_back(RenderPassInputDesc::depth(setIndex, binding, stageFlags));
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

  [[nodiscard]] static auto feedback(std::string pipelineName, uint32_t width,
                                     uint32_t height, VkFormat format)
      -> FeedbackFullscreenPassDesc {
    FeedbackFullscreenPassDesc desc{};
    desc.target.target =
        pipe::OffscreenTargetDesc::sampledColorOnly(width, height, format);
    desc.pipeline =
        pipe::GraphicsPipelineDesc::fullscreen(std::move(pipelineName));
    desc.clearColor(0.0F, 0.0F, 0.0F, 1.0F);
    return desc;
  }

  [[nodiscard]] static auto feedback(std::string pipelineName,
                                     VkExtent2D extent2D, VkFormat format)
      -> FeedbackFullscreenPassDesc {
    return FeedbackFullscreenPassDesc::feedback(pipelineName, extent2D.width,
                                                extent2D.height, format);
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
  auto uniform(uint32_t setIndex, uint32_t binding, T &buffer)
      -> FeedbackFullscreenPass & {
    for (uint32_t frame = 0; frame < descriptor_sets_.size(); ++frame) {
      descriptor_sets_[frame].at(setIndex).uniform(binding, buffer, frame);
    }
    return *this;
  }

  template <typename T>
  auto texture(uint32_t setIndex, uint32_t binding, T &texture)
      -> FeedbackFullscreenPass & {
    for (auto &frame : descriptor_sets_) {
      frame.at(setIndex).texture(binding, texture);
    }
    return *this;
  }

  template <typename T>
  auto storage(uint32_t setIndex, uint32_t binding, T &buffer)
      -> FeedbackFullscreenPass & {
    for (auto &frame : descriptor_sets_) {
      frame.at(setIndex).storage(binding, buffer);
    }
    return *this;
  }

  auto addSource(Pass &source) -> FeedbackFullscreenPass &;
  auto setSources(std::vector<std::reference_wrapper<Pass>> sources)
      -> FeedbackFullscreenPass &;

  [[nodiscard]] auto target() -> pipe::OffscreenTarget &;
  [[nodiscard]] auto target() const -> const pipe::OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t frameIndex)
      -> pipe::OffscreenTarget & override;
  [[nodiscard]] auto target(uint32_t frameIndex) const
      -> const pipe::OffscreenTarget & override;

  [[nodiscard]] auto historyTarget() -> pipe::OffscreenTarget &;
  [[nodiscard]] auto historyTarget() const -> const pipe::OffscreenTarget &;
  [[nodiscard]] auto historyTarget(uint32_t frameIndex)
      -> pipe::OffscreenTarget &;
  [[nodiscard]] auto historyTarget(uint32_t frameIndex) const
      -> const pipe::OffscreenTarget &;

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
  FeedbackFullscreenPassDesc desc_{};
  std::vector<std::reference_wrapper<Pass>> sources_{};
  std::unique_ptr<pipe::FrameHistoryTarget> target_{};
  std::unique_ptr<pipe::RenderPass> render_pass_{};
  std::vector<std::unique_ptr<Framebuffers>> framebuffers_{};
  pipe::DescriptorPool descriptor_pool_;
  std::vector<std::unique_ptr<pipe::DescriptorSetLayout>>
      descriptor_layouts_{};
  std::vector<std::vector<pipe::DescriptorSet>> descriptor_sets_{};
  std::unique_ptr<pipe::GraphicsPipeline> pipeline_{};

  // helpers
  void createTarget();
  void createRenderPass();
  void createFramebuffers();
  void validate(const FeedbackFullscreenPassDesc &desc) const;
  void createDescriptors();
  void createPipeline();

  [[nodiscard]] auto resolvedInputs() const -> std::vector<RenderPassInputDesc>;
};

} // namespace vkr::exec
