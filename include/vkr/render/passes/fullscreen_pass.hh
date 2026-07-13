#pragma once

#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/render/pass.hh"
#include "vkr/render/renderer.hh"
#include "vkr/resource/attachments/frame_buffer.hh"
#include "vkr/resource/targets/offscreen.hh"
#include <memory>
#include <utility>

namespace vkr::render {

class RasterPass;
class FullscreenPass;

struct FullscreenPassSource {
  enum class Type {
    Raster,
    Fullscreen,
  };

  Type type{Type::Raster};
  RasterPass *raster{nullptr};
  FullscreenPass *fullscreen{nullptr};

  FullscreenPassSource() = default;
  FullscreenPassSource(RasterPass &source);
  FullscreenPassSource(FullscreenPass &source);

  [[nodiscard]] auto target() -> resource::OffscreenTarget &;
  [[nodiscard]] auto target() const -> const resource::OffscreenTarget &;
};

struct FullscreenPassInputDesc {
  uint32_t binding{0};
  VkShaderStageFlags stageFlags{VK_SHADER_STAGE_FRAGMENT_BIT};
};

struct FullscreenPassDesc {
  resource::OffscreenTargetDesc target{};
  pipeline::DescriptorPoolDesc descriptorPool{};
  std::vector<VkClearValue> clearValues{};
  std::vector<FullscreenPassInputDesc> inputs{};
  pipeline::GraphicsPipelineDesc pipeline{};
};

class FullscreenPass : public RenderGraphPass {
public:
  FullscreenPass(Renderer &renderer, const core::Device &device,
                 const core::CommandPool &commandPool,
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

  [[nodiscard]] auto target() -> resource::OffscreenTarget &;
  [[nodiscard]] auto target() const -> const resource::OffscreenTarget &;

  [[nodiscard]] auto pipeline() noexcept -> pipeline::GraphicsPipeline * {
    return pipeline_.get();
  }

  [[nodiscard]] auto pipeline() const noexcept
      -> const pipeline::GraphicsPipeline * {
    return pipeline_.get();
  }

  [[nodiscard]] auto editablePipeline() noexcept
      -> pipeline::GraphicsPipeline * override {
    return pipeline_.get();
  }

  [[nodiscard]] auto editablePipeline() const noexcept
      -> const pipeline::GraphicsPipeline * override {
    return pipeline_.get();
  }

private:
  Renderer &renderer_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;

  FullscreenPassDesc desc_{};
  std::vector<FullscreenPassSource> sources_{};
  std::unique_ptr<resource::OffscreenTarget> target_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::unique_ptr<resource::FramebufferSet> framebuffers_{};
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
  [[nodiscard]] auto descriptorPoolDesc(
      const std::vector<FullscreenPassInputDesc> &inputs) const
      -> pipeline::DescriptorPoolDesc;
  [[nodiscard]] auto createDescriptorWrites(
      const std::vector<FullscreenPassInputDesc> &inputs)
      -> std::vector<pipeline::DescriptorSetWriteDesc>;
};

} // namespace vkr::render
