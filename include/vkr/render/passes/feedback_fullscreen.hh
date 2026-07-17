#pragma once

#include "vkr/render/passes/fullscreen_pass.hh"
#include "vkr/resource/targets/frame_history.hh"
#include <optional>

namespace vkr::render {

struct FeedbackFullscreenPassDesc {
  resource::FrameHistoryTargetDesc target{};
  std::vector<pipeline::DescriptorBinding> descriptorBindings{};
  pipeline::DescriptorPoolDesc descriptorPool{};
  std::vector<VkClearValue> clearValues{};
  std::optional<FullscreenPassInputDesc> historyInput{};
  std::vector<FullscreenPassInputDesc> inputs{};
  pipeline::GraphicsPipelineDesc pipeline{};
};

class FeedbackFullscreenPass final : public RenderGraphPass {
public:
  FeedbackFullscreenPass(Renderer &renderer, const core::Device &device,
                         const core::CommandPool &commandPool,
                         std::vector<FullscreenPassSource> sources = {});
  FeedbackFullscreenPass(Renderer &renderer, const core::Device &device,
                         const core::CommandPool &commandPool,
                         resource::ResourceManager &resourceManager,
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

  [[nodiscard]] auto target() -> resource::OffscreenTarget &;
  [[nodiscard]] auto target() const -> const resource::OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t frameIndex) -> resource::OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t frameIndex) const
      -> const resource::OffscreenTarget &;

  [[nodiscard]] auto historyTarget() -> resource::OffscreenTarget &;
  [[nodiscard]] auto historyTarget() const -> const resource::OffscreenTarget &;
  [[nodiscard]] auto historyTarget(uint32_t frameIndex)
      -> resource::OffscreenTarget &;
  [[nodiscard]] auto historyTarget(uint32_t frameIndex) const
      -> const resource::OffscreenTarget &;

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
  Renderer &renderer_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  resource::ResourceManager *resource_manager_{nullptr};

  FeedbackFullscreenPassDesc desc_{};
  std::vector<FullscreenPassSource> sources_{};
  std::unique_ptr<resource::FrameHistoryTarget> target_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::vector<std::unique_ptr<resource::FramebufferSet>> framebuffers_{};
  std::unique_ptr<pipeline::DescriptorPool> descriptor_pool_{};
  std::unique_ptr<pipeline::DescriptorSetLayout> descriptor_layout_{};
  std::unique_ptr<pipeline::DescriptorSets> descriptor_sets_{};
  std::unique_ptr<pipeline::GraphicsPipeline> pipeline_{};

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

} // namespace vkr::render
