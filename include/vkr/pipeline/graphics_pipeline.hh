#pragma once

#include "../core/device.hh"
#include "./descriptor/descriptor_binding.hh"
#include "./render_pass.hh"

namespace vkr {

class GraphicsPipeline {
public:
  explicit GraphicsPipeline(const Device &device, const RenderPass &renderPass,
                            DescriptorSetLayout &descriptorSetLayout,
                            const std::string &vertShaderPath,
                            const std::string &fragShaderPath);
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
  const Device &device;
  const RenderPass &renderPass;
  const DescriptorSetLayout &descriptorSetLayout;

  // componets
  VkPipelineLayout _pipelineLayout{VK_NULL_HANDLE};
  VkPipeline _graphicsPipeline{VK_NULL_HANDLE};
};
} // namespace vkr
