#pragma once

#include "../core/device.hh"
#include "../resources/manager.hh"
#include "./descriptors/set.hh"
#include "./render_pass.hh"
#include <string>
#include <vector>

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
                            const resource::ResourceManager &resourceManager,
                            const RenderPass &renderPass,
                            DescriptorSetLayout &descriptorSetLayout,
                            PipelineMode mode = PipelineMode::Default3D);
  ~GraphicsPipeline();
  GraphicsPipeline(const GraphicsPipeline &) = delete;
  GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

  bool rebuild(const std::string &vertShaderPath,
               const std::string &fragShaderPath);

  void requestRebuildFromSource(
      const std::string &vertSrc, const std::string &fragSrc,
      std::function<void(bool, const std::string &)> callback);

  bool flushPendingRebuild();

  [[nodiscard]] VkPipelineLayout pipelineLayout() const noexcept {
    return vk_pipeline_layout_;
  }
  [[nodiscard]] VkPipeline pipeline() const noexcept {
    return vk_graphics_pipeline_;
  }
  [[nodiscard]] bool isValid() const noexcept {
    return vk_graphics_pipeline_ != VK_NULL_HANDLE;
  }
  [[nodiscard]] const std::string &vertexSource() const noexcept {
    return vert_src_;
  }
  [[nodiscard]] const std::string &fragmentSource() const noexcept {
    return frag_src_;
  }

private:
  // dependencies
  const core::Device &device_;
  const resource::ResourceManager &resource_manager_;
  const RenderPass &render_pass_;
  DescriptorSetLayout &descriptor_set_layout_;
  PipelineMode mode_;

  // components
  VkPipelineLayout vk_pipeline_layout_{VK_NULL_HANDLE};
  VkPipeline vk_graphics_pipeline_{VK_NULL_HANDLE};
  std::string vert_src_path_;
  std::string frag_src_path_;
  std::string vert_src_;
  std::string frag_src_;

  // states
  struct PendingRebuild {
    std::string vertSrc;
    std::string fragSrc;
    std::vector<uint32_t> vertSpv;
    std::vector<uint32_t> fragSpv;
    std::function<void(bool, const std::string &)> callback;
  };
  std::optional<PendingRebuild> pending_rebuild_;
  std::mutex pending_mutex_;

  // helpers
  bool build(const std::vector<uint32_t> &vertSpv,
             const std::vector<uint32_t> &fragSpv);
  void destroyHandles();
  std::vector<uint32_t> compileGlsl(const std::string &src, bool isVertex,
                                    std::string &outError);
  void loadDefaultSources();
};

} // namespace vkr::pipeline
