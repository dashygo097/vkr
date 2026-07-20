#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/exec/render/pass.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/exec/render/attachments/frame_buffer.hh"
#include "vkr/scene/scene.hh"
#include "vkr/exec/render/targets/offscreen.hh"
#include <functional>
#include <memory>
#include <utility>
#include <variant>

namespace vkr::exec {

class RasterPass;
class SkyboxPass;
class FullscreenPass;
class FeedbackFullscreenPass;

struct FullscreenPassSource {
  using Source = std::variant<std::reference_wrapper<RasterPass>,
                              std::reference_wrapper<SkyboxPass>,
                              std::reference_wrapper<FullscreenPass>,
                              std::reference_wrapper<FeedbackFullscreenPass>>;

  explicit FullscreenPassSource(RasterPass &source);
  explicit FullscreenPassSource(SkyboxPass &source);
  explicit FullscreenPassSource(FullscreenPass &source);
  explicit FullscreenPassSource(FeedbackFullscreenPass &source);

  [[nodiscard]] auto target() -> OffscreenTarget &;
  [[nodiscard]] auto target() const -> const OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t frameIndex) -> OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t frameIndex) const
      -> const OffscreenTarget &;

private:
  Source source_;
};

struct FullscreenPassInputDesc {
  uint32_t binding{0};
  VkShaderStageFlags stageFlags{VK_SHADER_STAGE_FRAGMENT_BIT};

  [[nodiscard]] static auto
  image(uint32_t binding,
        VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FullscreenPassInputDesc {
    return {.binding = binding, .stageFlags = stageFlags};
  }
};

struct FullscreenPassDesc {
  OffscreenTargetDesc target{};
  std::vector<pipeline::DescriptorBinding> descriptorBindings{};
  pipeline::DescriptorPoolDesc descriptorPool{};
  std::vector<VkClearValue> clearValues{};
  std::vector<FullscreenPassInputDesc> inputs{};
  pipeline::GraphicsPipelineDesc pipeline{};
};

class FullscreenPass : public Pass {
public:
  FullscreenPass(Executor &executor, const core::Device &device,
                 const core::CommandPool &commandPool,
                 std::vector<FullscreenPassSource> sources = {});
  FullscreenPass(Executor &executor, const core::Device &device,
                 const core::CommandPool &commandPool,
                 scene::Scene &scene,
                 std::vector<FullscreenPassSource> sources = {});
  ~FullscreenPass() override;

  FullscreenPass(const FullscreenPass &) = delete;
  auto operator=(const FullscreenPass &) -> FullscreenPass & = delete;

  void create() override;
  void destroy() override;
  void update(const FullscreenPassDesc &desc);
  void record() override;

  auto addSource(FullscreenPassSource source) -> FullscreenPass &;
  auto setSources(std::vector<FullscreenPassSource> sources)
      -> FullscreenPass &;

  [[nodiscard]] auto target() -> OffscreenTarget &;
  [[nodiscard]] auto target() const -> const OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t) -> OffscreenTarget & {
    return target();
  }
  [[nodiscard]] auto target(uint32_t) const
      -> const OffscreenTarget & {
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
  Executor &executor_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  scene::Scene *scene_{nullptr};

  FullscreenPassDesc desc_{};
  std::vector<FullscreenPassSource> sources_{};
  std::unique_ptr<OffscreenTarget> target_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::unique_ptr<FramebufferSet> framebuffers_{};
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

} // namespace vkr::exec
