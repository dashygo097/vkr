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
#include "vkr/resource/manager.hh"
#include "vkr/resource/targets/offscreen.hh"
#include <memory>

namespace vkr::render {

struct RasterPassDesc {
  resource::OffscreenTargetDesc target{};
  std::vector<pipeline::DescriptorBinding> descriptorBindings{};
  pipeline::DescriptorPoolDesc descriptorPool{};
  std::vector<VkClearValue> clearValues{};
  pipeline::GraphicsPipelineDesc pipeline{};
};

class RasterPass final : public RenderGraphPass {
public:
  RasterPass(Renderer &renderer, const core::Device &device,
             const core::CommandPool &commandPool,
             resource::ResourceManager &resourceManager);
  ~RasterPass() override;

  RasterPass(const RasterPass &) = delete;
  auto operator=(const RasterPass &) -> RasterPass & = delete;

  void create() override;
  void destroy() override;
  void update(const RasterPassDesc &desc);
  void record() override;

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
  // dependencies
  Renderer &renderer_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  resource::ResourceManager &resource_manager_;

  // components
  RasterPassDesc desc_{};
  std::unique_ptr<resource::OffscreenTarget> target_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::unique_ptr<resource::FramebufferSet> framebuffers_{};
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

  [[nodiscard]] auto createDescriptorWrites() const
      -> std::vector<pipeline::DescriptorSetWriteDesc>;
};

} // namespace vkr::render
