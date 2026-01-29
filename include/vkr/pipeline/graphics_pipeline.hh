#pragma once

#include "../core/device.hh"
#include "../resources/manager.hh"
#include "./descriptor/descriptor.hh"
#include "./render_pass.hh"

namespace vkr::pipeline {
enum class PipelineMode {
  Default3D,
  NoVertices,
};

class GraphicsPipeline {
public:
  explicit GraphicsPipeline(const core::Device &device,
                            const resource::ResourceManager &resourceManager,
                            const pipeline::RenderPass &renderPass,
                            DescriptorSetLayout &descriptorSetLayout,
                            const std::string &vertShaderPath,
                            const std::string &fragShaderPath,
                            PipelineMode mode = PipelineMode::Default3D);
  ~GraphicsPipeline();

  GraphicsPipeline(const GraphicsPipeline &) = delete;
  GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

  [[nodiscard]] VkPipelineLayout pipelineLayout() const noexcept {
    return _pipelineLayout;
  }
  [[nodiscard]] VkPipeline pipeline() const noexcept {
    return _graphicsPipeline;
  }

private:
  // dependencies
  const core::Device &device;
  const resource::ResourceManager &resourceManager;
  const RenderPass &renderPass;
  const DescriptorSetLayout &descriptorSetLayout;

  // componets
  VkPipelineLayout _pipelineLayout{VK_NULL_HANDLE};
  VkPipeline _graphicsPipeline{VK_NULL_HANDLE};
};
} // namespace vkr::pipeline
