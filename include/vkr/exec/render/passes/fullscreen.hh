#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/exec/render/attachments/frame_buffer.hh"
#include "vkr/exec/render/executor.hh"
#include "vkr/exec/render/pass.hh"
#include "vkr/exec/render/targets/offscreen.hh"
#include "vkr/pipeline/descriptors/layout.hh"
#include "vkr/pipeline/descriptors/pool.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/scene/scene.hh"
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace vkr::exec {

class RasterPass;
class FullscreenPass;
class FeedbackFullscreenPass;

struct FullscreenPassSource {
  using Source = std::variant<std::reference_wrapper<RasterPass>,
                              std::reference_wrapper<FullscreenPass>,
                              std::reference_wrapper<FeedbackFullscreenPass>>;

  explicit FullscreenPassSource(RasterPass &source);
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
  pipeline::GraphicsPipelineDesc graphicsPipeline{};

  auto targetDesc(OffscreenTargetDesc desc) -> FullscreenPassDesc & {
    target = std::move(desc);
    return *this;
  }

  auto color(uint32_t width, uint32_t height, VkFormat format)
      -> FullscreenPassDesc & {
    target.color.width = width;
    target.color.height = height;
    target.color.format = format;
    return *this;
  }

  auto color(ColorAttachmentDesc desc) -> FullscreenPassDesc & {
    target.color = std::move(desc);
    return *this;
  }

  auto colorUsage(VkImageUsageFlags usage) -> FullscreenPassDesc & {
    target.color.usage = usage;
    return *this;
  }

  auto sampledColor(bool enabled = true) -> FullscreenPassDesc & {
    if (enabled) {
      target.color.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
      target.color.createSampler = true;
      if (target.color.finalLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        target.color.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
      return *this;
    }

    target.color.usage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
    target.color.createSampler = false;
    return *this;
  }

  auto colorFinalLayout(VkImageLayout layout) -> FullscreenPassDesc & {
    target.color.finalLayout = layout;
    return *this;
  }

  auto colorSampler(resource::SamplerDesc desc) -> FullscreenPassDesc & {
    target.color.sampler = std::move(desc);
    target.color.createSampler = true;
    return *this;
  }

  auto depth(VkFormat format) -> FullscreenPassDesc & {
    target.depth = DepthAttachmentDesc{
        .width = target.color.width,
        .height = target.color.height,
        .format = format,
    };
    return *this;
  }

  auto depth(uint32_t width, uint32_t height, VkFormat format)
      -> FullscreenPassDesc & {
    target.depth = DepthAttachmentDesc{
        .width = width,
        .height = height,
        .format = format,
    };
    return *this;
  }

  auto disableDepthAttachment() -> FullscreenPassDesc & {
    target.depth.reset();
    return *this;
  }

  auto descriptor(pipeline::DescriptorBinding binding)
      -> FullscreenPassDesc & {
    descriptorBindings.push_back(std::move(binding));
    return *this;
  }

  auto uniform(uint32_t binding, std::string name,
               VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
               uint32_t descriptorCount = 1) -> FullscreenPassDesc & {
    return descriptor({.name = std::move(name),
                       .layout = {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                  descriptorCount, stageFlags}});
  }

  auto texture(uint32_t binding, std::string name,
               VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
               uint32_t descriptorCount = 1) -> FullscreenPassDesc & {
    return descriptor(
        {.name = std::move(name),
         .layout = {binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    descriptorCount, stageFlags}});
  }

  auto input(uint32_t binding,
             VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
      -> FullscreenPassDesc & {
    inputs.push_back(FullscreenPassInputDesc::image(binding, stageFlags));
    return *this;
  }

  auto input(FullscreenPassInputDesc desc) -> FullscreenPassDesc & {
    inputs.push_back(desc);
    return *this;
  }

  auto clearColor(float r, float g, float b, float a)
      -> FullscreenPassDesc & {
    clearValues.push_back(VkClearValue{.color = {{r, g, b, a}}});
    return *this;
  }

  auto clearDepth(float depthValue = 1.0f, uint32_t stencil = 0)
      -> FullscreenPassDesc & {
    clearValues.push_back(
        VkClearValue{.depthStencil = {depthValue, stencil}});
    return *this;
  }

  auto pipelineDesc(pipeline::GraphicsPipelineDesc desc)
      -> FullscreenPassDesc & {
    graphicsPipeline = std::move(desc);
    return *this;
  }

  auto pipeline(std::string name) -> FullscreenPassDesc & {
    graphicsPipeline.setName(std::move(name));
    return *this;
  }

  auto vertexInput(scene::VertexInputDesc desc) -> FullscreenPassDesc & {
    graphicsPipeline.vertexInputDesc(std::move(desc));
    return *this;
  }

  auto shader(pipeline::GraphicsShaderStageDesc shaderDesc)
      -> FullscreenPassDesc & {
    graphicsPipeline.shader(std::move(shaderDesc));
    return *this;
  }

  auto vertexShader(resource::ShaderModuleDesc shaderDesc,
                    std::string entryPoint = "main") -> FullscreenPassDesc & {
    graphicsPipeline.vertexShader(std::move(shaderDesc),
                                  std::move(entryPoint));
    return *this;
  }

  auto fragmentShader(resource::ShaderModuleDesc shaderDesc,
                      std::string entryPoint = "main")
      -> FullscreenPassDesc & {
    graphicsPipeline.fragmentShader(std::move(shaderDesc),
                                    std::move(entryPoint));
    return *this;
  }

  auto depthTest(VkBool32 testEnable = VK_TRUE,
                 VkBool32 writeEnable = VK_TRUE,
                 VkCompareOp compareOp = VK_COMPARE_OP_LESS)
      -> FullscreenPassDesc & {
    graphicsPipeline.depth(testEnable, writeEnable, compareOp);
    return *this;
  }

  auto disableDepthTest() -> FullscreenPassDesc & {
    graphicsPipeline.disableDepth();
    return *this;
  }

  auto readOnlyDepthTest() -> FullscreenPassDesc & {
    graphicsPipeline.readOnlyDepth();
    return *this;
  }

  auto rasterize(pipeline::GraphicsRasterizationDesc desc)
      -> FullscreenPassDesc & {
    graphicsPipeline.rasterize(desc);
    return *this;
  }

  auto noCull() -> FullscreenPassDesc & {
    graphicsPipeline.noCull();
    return *this;
  }

  auto blend(pipeline::GraphicsColorBlendDesc desc) -> FullscreenPassDesc & {
    graphicsPipeline.blend(std::move(desc));
    return *this;
  }

  auto alphaBlend() -> FullscreenPassDesc & {
    graphicsPipeline.alphaBlend();
    return *this;
  }
};

class FullscreenPass : public Pass {
public:
  FullscreenPass(Executor &executor, const core::Device &device,
                 const core::CommandPool &commandPool,
                 std::vector<FullscreenPassSource> sources = {});
  FullscreenPass(Executor &executor, const core::Device &device,
                 const core::CommandPool &commandPool, scene::Scene &scene,
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
  [[nodiscard]] auto target(uint32_t) -> OffscreenTarget & { return target(); }
  [[nodiscard]] auto target(uint32_t) const -> const OffscreenTarget & {
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
  // dependencies
  Executor &executor_;
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  scene::Scene *scene_{nullptr};

  // components
  FullscreenPassDesc desc_{};
  std::vector<FullscreenPassSource> sources_{};
  std::unique_ptr<OffscreenTarget> target_{};
  std::unique_ptr<pipeline::RenderPass> render_pass_{};
  std::unique_ptr<FramebufferSet> framebuffers_{};
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
