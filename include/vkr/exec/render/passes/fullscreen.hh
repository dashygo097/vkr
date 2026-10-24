#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/exec/capability.hh"
#include "vkr/exec/pass.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/exec/render/framebuffers.hh"
#include "vkr/exec/render/passes/input.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/pipeline/targets/offscreen.hh"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace vkr::exec {

struct FullscreenPassDesc {
  pipe::OffscreenTargetDesc target{};
  std::vector<pipe::DescriptorSetLayoutDesc> descriptorLayouts{};
  std::vector<VkClearValue> clearValues{};
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
      -> FullscreenPassDesc & {
    layout(setIndex).uniform(binding, stages);
    return *this;
  }

  auto texture(uint32_t setIndex, uint32_t binding,
               VkShaderStageFlags stages = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FullscreenPassDesc & {
    layout(setIndex).texture(binding, stages);
    return *this;
  }

  auto input(uint32_t setIndex, uint32_t binding,
             VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FullscreenPassDesc & {
    inputs.push_back(RenderPassInputDesc::color(setIndex, binding, stageFlags));
    return *this;
  }

  auto inputDepth(uint32_t setIndex, uint32_t binding,
                  VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FullscreenPassDesc & {
    inputs.push_back(RenderPassInputDesc::depth(setIndex, binding, stageFlags));
    return *this;
  }

  auto input(RenderPassInputDesc desc) -> FullscreenPassDesc & {
    inputs.push_back(desc);
    return *this;
  }

  auto clearColor(float r, float g, float b, float a) -> FullscreenPassDesc & {
    clearValues.push_back(VkClearValue{.color = {{r, g, b, a}}});
    return *this;
  }

  auto clearDepth(float depthValue = 1.0f, uint32_t stencil = 0)
      -> FullscreenPassDesc & {
    clearValues.push_back(VkClearValue{.depthStencil = {depthValue, stencil}});
    return *this;
  }

  [[nodiscard]] static auto offscreen(uint32_t width, uint32_t height,
                                      VkFormat format) -> FullscreenPassDesc {
    FullscreenPassDesc desc{};
    desc.target =
        pipe::OffscreenTargetDesc::colorOnly(width, height, format);
    desc.pipeline = pipe::GraphicsPipelineDesc::fullscreen("fullscreen");
    return desc;
  }

  [[nodiscard]] static auto offscreen(VkExtent2D extent2D, VkFormat format)
      -> FullscreenPassDesc {
    return FullscreenPassDesc::offscreen(extent2D.width, extent2D.height,
                                         format);
  }

  [[nodiscard]] static auto sampledOffscreen(uint32_t width, uint32_t height,
                                             VkFormat format)
      -> FullscreenPassDesc {
    FullscreenPassDesc desc{};
    desc.target =
        pipe::OffscreenTargetDesc::sampledColorOnly(width, height, format);
    desc.pipeline = pipe::GraphicsPipelineDesc::fullscreen("fullscreen");
    return desc;
  }

  [[nodiscard]] static auto sampledOffscreen(VkExtent2D extent2D,
                                             VkFormat format)
      -> FullscreenPassDesc {
    return FullscreenPassDesc::sampledOffscreen(extent2D.width, extent2D.height,
                                                format);
  }

  [[nodiscard]] static auto postProcess(std::string pipelineName,
                                        uint32_t width, uint32_t height,
                                        VkFormat format) -> FullscreenPassDesc {
    FullscreenPassDesc desc{};
    desc.target =
        pipe::OffscreenTargetDesc::sampledColorOnly(width, height, format);
    desc.pipeline =
        pipe::GraphicsPipelineDesc::fullscreen(std::move(pipelineName));
    desc.clearColor(0.0F, 0.0F, 0.0F, 1.0F);
    return desc;
  }

  [[nodiscard]] static auto postProcess(std::string pipelineName,
                                        VkExtent2D extent2D, VkFormat format)
      -> FullscreenPassDesc {
    return FullscreenPassDesc::postProcess(pipelineName, extent2D.width,
                                           extent2D.height, format);
  }
};

class FullscreenPass : public Pass,
                       public GraphicsPipelineCapability,
                       public RenderTargetCapability {
public:
  explicit FullscreenPass(
      RenderExecutor &executor, const core::Device &device,
      std::vector<std::reference_wrapper<Pass>> sources = {});
  ~FullscreenPass() override;

  FullscreenPass(const FullscreenPass &) = delete;
  auto operator=(const FullscreenPass &) -> FullscreenPass & = delete;

  void create() override;
  void destroy() noexcept override;
  void update(const FullscreenPassDesc &desc);
  void record() override;

  template <typename T>
  auto uniform(uint32_t setIndex, uint32_t binding, T &buffer)
      -> FullscreenPass & {
    for (uint32_t frame = 0; frame < descriptor_sets_.size(); ++frame) {
      descriptor_sets_[frame].at(setIndex).uniform(binding, buffer, frame);
    }
    return *this;
  }

  template <typename T>
  auto texture(uint32_t setIndex, uint32_t binding, T &texture)
      -> FullscreenPass & {
    for (auto &frame : descriptor_sets_) {
      frame.at(setIndex).texture(binding, texture);
    }
    return *this;
  }

  template <typename T>
  auto storage(uint32_t setIndex, uint32_t binding, T &buffer)
      -> FullscreenPass & {
    for (auto &frame : descriptor_sets_) {
      frame.at(setIndex).storage(binding, buffer);
    }
    return *this;
  }

  auto addSource(Pass &source) -> FullscreenPass &;
  auto setSources(std::vector<std::reference_wrapper<Pass>> sources)
      -> FullscreenPass &;

  [[nodiscard]] auto target() -> pipe::OffscreenTarget &;
  [[nodiscard]] auto target() const -> const pipe::OffscreenTarget &;
  [[nodiscard]] auto target(uint32_t) -> pipe::OffscreenTarget & override {
    return target();
  }
  [[nodiscard]] auto target(uint32_t) const
      -> const pipe::OffscreenTarget & override {
    return target();
  }

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
  FullscreenPassDesc desc_{};
  std::vector<std::reference_wrapper<Pass>> sources_{};
  std::unique_ptr<pipe::OffscreenTarget> target_{};
  std::unique_ptr<pipe::RenderPass> render_pass_{};
  std::unique_ptr<Framebuffers> framebuffers_{};
  pipe::DescriptorPool descriptor_pool_;
  std::vector<std::unique_ptr<pipe::DescriptorSetLayout>>
      descriptor_layouts_{};
  std::vector<std::vector<pipe::DescriptorSet>> descriptor_sets_{};
  std::unique_ptr<pipe::GraphicsPipeline> pipeline_{};

  // helpers
  void createTarget();
  void createRenderPass();
  void createFramebuffers();
  void validate(const FullscreenPassDesc &desc) const;
  void createDescriptors();
  void createPipeline();

  [[nodiscard]] auto resolvedInputs() const -> std::vector<RenderPassInputDesc>;
};

} // namespace vkr::exec
