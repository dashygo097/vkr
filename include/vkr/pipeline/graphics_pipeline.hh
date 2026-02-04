#pragma once

#include "../core/device.hh"
#include "../resources/manager.hh"
#include "./descriptors/set.hh"
#include "./render_pass.hh"

namespace vkr::pipeline {
enum class PipelineMode {
  Default2D,
  Textured2D,
  Default3D,
  Textured3D,
  NoVertices,
};

class GraphicsPipeline {
public:
  explicit GraphicsPipeline(const core::Device &device,
                            const resource::ResourceManager &resource_manager_,
                            const pipeline::RenderPass &render_pass_,
                            DescriptorSetLayout &descriptor_set_layout_,
                            const std::string &vertShaderPath,
                            const std::string &fragShaderPath,
                            PipelineMode mode = PipelineMode::Default3D);
  ~GraphicsPipeline();

  GraphicsPipeline(const GraphicsPipeline &) = delete;
  GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

  [[nodiscard]] VkPipelineLayout pipelineLayout() const noexcept {
    return vk_pipeline_layout_;
  }
  [[nodiscard]] VkPipeline pipeline() const noexcept {
    return vk_graphics_pipeline;
  }

private:
  // dependencies
  const core::Device &device_;
  const resource::ResourceManager &resource_manager_;
  const RenderPass &render_pass_;
  const DescriptorSetLayout &descriptor_set_layout_;

  // componets
  VkPipelineLayout vk_pipeline_layout_{VK_NULL_HANDLE};
  VkPipeline vk_graphics_pipeline{VK_NULL_HANDLE};
};
} // namespace vkr::pipeline
