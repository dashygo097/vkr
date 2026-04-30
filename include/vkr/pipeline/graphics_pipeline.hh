#pragma once
#include "../core/device.hh"
#include "../resources/manager.hh"
#include "./descriptors/set.hh"
#include "./render_pass.hh"
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace vkr::pipeline {

enum class PipelineMode {
  Default2D,
  Textured2D,
  Default3D,
  Normal3D,
  Textured3D,
  NoVertices,
};

class GraphicsPipeline {
public:
  explicit GraphicsPipeline(const core::Instance &instance,
                            const core::Device &device,
                            const resource::ResourceManager &resourceManager,
                            const RenderPass &renderPass,
                            DescriptorSetLayout &descriptorSetLayout,
                            PipelineMode mode = PipelineMode::Default3D);
  ~GraphicsPipeline();
  GraphicsPipeline(const GraphicsPipeline &) = delete;
  auto operator=(const GraphicsPipeline &) -> GraphicsPipeline & = delete;

  auto rebuild(const std::string &vertShaderPath,
               const std::string &fragShaderPath) -> bool;

  void requestRebuildFromSource(
      const std::string &vertSrc, const std::string &fragSrc,
      std::function<void(bool, const std::string &)> callback);

  auto flushPendingRebuild() -> bool;

  void buildOffscreen(VkRenderPass offscreenRenderPass);
  void destroyOffscreenHandles();

  [[nodiscard]] auto pipelineLayout() const noexcept -> VkPipelineLayout {
    return vk_pipeline_layout_;
  }
  [[nodiscard]] auto pipeline() const noexcept -> VkPipeline {
    return vk_graphics_pipeline_;
  }
  [[nodiscard]] auto offscreenPipelineLayout() const noexcept
      -> VkPipelineLayout {
    return vk_offscreen_layout_;
  }
  [[nodiscard]] auto offscreenPipeline() const noexcept -> VkPipeline {
    return vk_offscreen_pipeline_;
  }
  [[nodiscard]] auto vertexSource() const noexcept -> const std::string & {
    return vert_src_;
  }
  [[nodiscard]] auto fragmentSource() const noexcept -> const std::string & {
    return frag_src_;
  }

private:
  // dependencies
  const core::Instance &instance_;
  const core::Device &device_;
  const resource::ResourceManager &resource_manager_;
  const RenderPass &render_pass_;
  DescriptorSetLayout &descriptor_set_layout_;
  PipelineMode mode_;

  // components
  VkPipelineLayout vk_pipeline_layout_{VK_NULL_HANDLE};
  VkPipeline vk_graphics_pipeline_{VK_NULL_HANDLE};

  VkPipelineLayout vk_offscreen_layout_{VK_NULL_HANDLE};
  VkPipeline vk_offscreen_pipeline_{VK_NULL_HANDLE};
  VkRenderPass offscreen_render_pass_{VK_NULL_HANDLE};

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
  auto build(const std::vector<uint32_t> &vertSpv,
             const std::vector<uint32_t> &fragSpv) -> bool;

  auto buildInto(const std::vector<uint32_t> &vertSpv,
                 const std::vector<uint32_t> &fragSpv,
                 VkRenderPass targetRenderPass, VkPipelineLayout &outLayout,
                 VkPipeline &outPipeline) -> bool;

  void destroyHandles();

  auto compileGlsl(const std::string &src, bool isVertex, std::string &outError)
      -> std::vector<uint32_t>;
  void loadDefaultSources();
};

} // namespace vkr::pipeline
