#pragma once

#include "vkr/core/command/buffers.hh"
#include "vkr/pipeline/descriptors/set.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/pipeline/render_pass.hh"
#include "vkr/resource/buffer/uniform_buffer.hh"
#include "vkr/scene/scene.hh"
#include "vkr/ui/selection.hh"
#include <array>
#include <imgui.h>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace vkr::ui {

// Inspector implementation. Does not own the inspected images.
class TexturePreview final {
public:
  TexturePreview(const core::Device &device,
                 const pipeline::RenderPass &renderPass,
                 const core::CommandBuffers &commandBuffers,
                 const scene::Scene &scene, const Selection &selection);
  ~TexturePreview();

  TexturePreview(const TexturePreview &) = delete;
  auto operator=(const TexturePreview &) -> TexturePreview & = delete;

  // Called after the frame fence has completed, outside a render pass.
  void prepare(uint32_t frameIndex);
  void render();

private:
  struct Image {
    resource::ImageDesc image{};
    resource::ImageViewDesc view{};
    VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
    VkFormatFeatureFlags features{0};
    bool cubemap{false};
  };

  // Matches the fragment shader's std140 block.
  struct alignas(16) Parameters {
    int32_t channel{0};
    int32_t depth{0};
    int32_t toneMap{0};
    int32_t encodeSrgb{0};
    float exposure{0.0f};
    float depthMin{0.0f};
    float depthMax{1.0f};
    float padding{0.0f};
  };

  struct Probe {
    Selection selection{};
    resource::ImageViewDesc view{};
    VkImageView originalView{VK_NULL_HANDLE};
    uint32_t x{0};
    uint32_t y{0};
  };

  struct Frame {
    explicit Frame(const core::Device &device);

    // Thumbnail and popup never overwrite one another's GPU state.
    std::array<resource::ImageView, 2> views;
    std::array<resource::UniformBuffer<Parameters>, 2> parameters;
    resource::Buffer readback;
    std::optional<Probe> probe{};
    bool stale{true};
  };

  struct Draw {
    std::reference_wrapper<const pipeline::GraphicsPipeline> pipeline;
    std::reference_wrapper<const pipeline::DescriptorSets> images;
    std::reference_wrapper<const pipeline::DescriptorSets> parameters;
    uint32_t index{0};
  };

  const core::Device &device_;
  const pipeline::RenderPass &render_pass_;
  const core::CommandBuffers &command_buffers_;
  const scene::Scene &scene_;
  const Selection &selection_;

  pipeline::DescriptorSetLayout image_layout_;
  pipeline::DescriptorSetLayout parameter_layout_;
  pipeline::DescriptorPool descriptor_pool_;
  pipeline::DescriptorSets image_sets_;
  pipeline::DescriptorSets parameter_sets_;
  resource::Sampler nearest_sampler_;
  resource::Sampler linear_sampler_;
  pipeline::GraphicsPipeline pipeline_;
  std::vector<std::unique_ptr<Frame>> frames_{};
  // Stable addresses for ImGui's callbacks; populated only during construction.
  std::vector<Draw> draws_{};

  Selection inspected_{};
  VkImageView inspected_view_{VK_NULL_HANDLE};
  uint32_t frame_index_{0};
  int mip_{0};
  int layer_{0};
  int channel_{0};
  float exposure_{0.0f};
  float depth_min_{0.0f};
  float depth_max_{1.0f};
  float zoom_{1.0f};
  ImVec2 pan_{};
  bool fit_{true};
  bool linear_{true};
  bool tone_map_{true};
  bool srgb_{false};
  bool flip_y_{false};
  std::optional<Probe> requested_probe_{};
  std::optional<Probe> completed_probe_{};
  std::optional<std::array<float, 4>> texel_{};

  [[nodiscard]] auto image() const -> std::optional<Image>;
  [[nodiscard]] auto originalView() const -> VkImageView;
  [[nodiscard]] auto validate(const Image &image) const -> std::string_view;
  void createPipeline();
  void controls(const Image &image, bool enlarged);
  void draw(const Image &image, ImVec2 available, bool interactive);
  static void bind(const ImDrawList *, const ImDrawCmd *command);
};

} // namespace vkr::ui
