#pragma once

#include "vkr/core/command/pool.hh"
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
#include <string>

namespace vkr::render {

struct SkyboxPassDesc {
  resource::OffscreenTargetDesc target{};
  std::string uniformName{"skybox"};
  std::string cubemapName{"skybox"};
  std::string meshName{"skybox"};
  pipeline::DescriptorPoolDesc descriptorPool{};
  std::vector<VkClearValue> clearValues{};
  pipeline::GraphicsPipelineDesc pipeline{};
  bool configureSkyboxPipelineState{true};
};

class SkyboxPass final : public RenderGraphPass {
public:
  SkyboxPass(Renderer &renderer, const core::Device &device,
             const core::CommandPool &commandPool,
             resource::ResourceManager &resourceManager);
  ~SkyboxPass() override;

  SkyboxPass(const SkyboxPass &) = delete;
  auto operator=(const SkyboxPass &) -> SkyboxPass & = delete;

  void create() override;
  void destroy() override;
  void update(const SkyboxPassDesc &desc);
  void record() override;

  [[nodiscard]] auto target() -> resource::OffscreenTarget &;
  [[nodiscard]] auto target() const -> const resource::OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t) -> resource::OffscreenTarget & {
    return target();
  }
  [[nodiscard]] auto target(uint32_t) const
      -> const resource::OffscreenTarget & {
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
  Renderer &renderer_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  resource::ResourceManager &resource_manager_;

  SkyboxPassDesc desc_{};
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

  [[nodiscard]] auto descriptorPoolDesc() const -> pipeline::DescriptorPoolDesc;
  [[nodiscard]] auto createDescriptorWrites() const
      -> std::vector<pipeline::DescriptorSetWriteDesc>;
};

} // namespace vkr::render
