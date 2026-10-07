#include <cstdint>
#include <ctime>
#include <glm/glm.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

namespace {

struct UniformBufferShaderToyObject {
  alignas(16) glm::vec3 iResolution;
  alignas(4) float iTime;
  alignas(4) float iTimeDelta;
  alignas(4) float iFrameRate;
  alignas(4) int iFrame;
  alignas(16) glm::vec4 iMouse;
  alignas(16) glm::vec4 iDate;
  alignas(16) glm::vec4 iChannelTime;
  alignas(16) glm::vec4 iChannelResolution[4];
};

} // namespace

class ShaderToyApp : public vkr::exec::RenderApplication {
private:
  static constexpr uint32_t kShaderToyChannelCount = 4;
  static constexpr std::string_view kShaderToyUniformName{"shadertoy"};
  static constexpr std::string_view kFallbackTextureName{"shadertoy.black"};

  uint64_t shadertoy_pipeline_revision_{0};
  uint64_t shadertoy_frame_offset_{0};
  float shadertoy_time_offset_{0.0f};
  std::vector<std::reference_wrapper<vkr::exec::GraphicsPipelineCapability>>
      shader_pipelines_{};

  glm::vec4 shadertoyMouse() const {
    const auto mousePosition = viewportMousePosition();
    const bool mouseDown =
        inputTracer->isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);

    return {mousePosition.x, mousePosition.y,
            mouseDown ? mousePosition.x : 0.0f,
            mouseDown ? mousePosition.y : 0.0f};
  }

  [[nodiscard]] static auto channelInput(uint32_t channel)
      -> vkr::exec::RenderPassInputDesc {
    return vkr::exec::RenderPassInputDesc::color(1U + channel);
  }

  [[nodiscard]] static auto hasChannel(const std::vector<uint32_t> &channels,
                                       uint32_t channel) -> bool {
    for (uint32_t existing : channels) {
      if (existing == channel) {
        return true;
      }
    }

    return false;
  }

  [[nodiscard]] static auto
  fallbackChannels(std::optional<uint32_t> historyChannel,
                   const std::vector<uint32_t> &sourceChannels)
      -> std::vector<uint32_t> {
    std::vector<uint32_t> channels{};
    channels.reserve(4);

    for (uint32_t channel = 0; channel < kShaderToyChannelCount; ++channel) {
      if (historyChannel && *historyChannel == channel) {
        continue;
      }

      if (hasChannel(sourceChannels, channel)) {
        continue;
      }

      channels.push_back(channel);
    }

    return channels;
  }

  [[nodiscard]] auto shadertoyPipeline(const std::string &name,
                                       const std::string &fragmentShader) const
      -> vkr::pipeline::GraphicsPipelineDesc {
    auto pipeline = vkr::pipeline::GraphicsPipelineDesc::fullscreen(name);
    pipeline
        .vertexShader(vkr::resource::ShaderModuleDesc::vertexGlslFile(
            assetSystem->resolveApp("shaders/shadertoy/shadertoy.vert")
                .string()))
        .fragmentShader(vkr::resource::ShaderModuleDesc::fragmentGlslFile(
            assetSystem->resolveApp("shaders/shadertoy/" + fragmentShader)
                .string()));
    return pipeline;
  }

  [[nodiscard]] auto feedbackDesc(const std::string &name,
                                  const std::string &fragmentShader,
                                  std::optional<uint32_t> historyChannel,
                                  const std::vector<uint32_t> &sourceChannels)
      -> vkr::exec::FeedbackFullscreenPassDesc {
    const auto fallback = fallbackChannels(historyChannel, sourceChannels);

    auto desc = vkr::exec::FeedbackFullscreenPassDesc::feedback(
        swapchain->width(), swapchain->height(), VK_FORMAT_R16G16B16A16_SFLOAT,
        name);

    desc.uniform(0);
    for (uint32_t channel : fallback) {
      desc.texture(channelInput(channel).binding);
    }

    if (historyChannel) {
      desc.history(channelInput(*historyChannel));
    }

    for (uint32_t channel : sourceChannels) {
      desc.input(channelInput(channel));
    }

    desc.pipeline = shadertoyPipeline(name, fragmentShader);
    return desc;
  }

  [[nodiscard]] auto imageDesc(const std::vector<uint32_t> &sourceChannels)
      -> vkr::exec::FullscreenPassDesc {
    const auto fallback = fallbackChannels(std::nullopt, sourceChannels);

    auto desc = vkr::exec::FullscreenPassDesc::postProcess(
        swapchain->width(), swapchain->height(), VK_FORMAT_R16G16B16A16_SFLOAT,
        "shadertoy.image");

    desc.uniform(0);
    for (uint32_t channel : fallback) {
      desc.texture(channelInput(channel).binding);
    }

    for (uint32_t channel : sourceChannels) {
      desc.input(channelInput(channel));
    }

    desc.pipeline = shadertoyPipeline("shadertoy.image", "image.frag");
    return desc;
  }

  template <typename PassT>
  void bindChannels(PassT &pass, std::optional<uint32_t> historyChannel,
                    const std::vector<uint32_t> &sourceChannels) {
    pass.uniform(0, scene->uniformBuffer(kShaderToyUniformName));
    for (uint32_t channel : fallbackChannels(historyChannel, sourceChannels)) {
      pass.texture(channelInput(channel).binding,
                   scene->texture(kFallbackTextureName));
    }
  }

  [[nodiscard]] auto viewportMousePosition() const -> glm::vec2 {
    const auto cursor = inputTracer->cursorPosition();

    if (ui().layoutMode() == vkr::ui::LayoutMode::FullScreen) {
      return {static_cast<float>(cursor.x),
              static_cast<float>(ctx.window.height - cursor.y)};
    }

    const auto viewport = ui().viewport();
    if (!ui().viewportFocused() || viewport.width <= 0.0f ||
        viewport.height <= 0.0f) {
      return {0.0f, 0.0f};
    }

    const float localX = static_cast<float>(cursor.x) - viewport.x;
    const float localY = static_cast<float>(cursor.y) - viewport.y;
    const float scaledX =
        localX * static_cast<float>(ctx.window.width) / viewport.width;
    const float scaledY = (viewport.height - localY) *
                          static_cast<float>(ctx.window.height) /
                          viewport.height;

    return {scaledX, scaledY};
  }

  [[nodiscard]] auto isViewportMouseActive() const -> bool {
    if (ui().layoutMode() == vkr::ui::LayoutMode::FullScreen) {
      return true;
    }

    const auto viewport = ui().viewport();
    return ui().viewportFocused() && viewport.width > 0.0f &&
           viewport.height > 0.0f;
  }

  [[nodiscard]] auto shadertoyPipelineRevision() -> uint64_t {
    uint64_t hash = 1469598103934665603ULL;

    for (const auto &capability : shader_pipelines_) {
      auto pipeline = capability.get().editablePipeline();
      if (!pipeline) {
        continue;
      }

      hash ^= pipeline->get().revision() + 0x9E3779B97F4A7C15ULL +
              (hash << 6U) + (hash >> 2U);
    }

    return hash;
  }

  void createResources() override {
    scene->createUniformBuffer<UniformBufferShaderToyObject>(
        std::string(kShaderToyUniformName), {});
    scene->createTexture(
        std::string(kFallbackTextureName),
        vkr::scene::TextureDesc::sampled2D(1, 1, VK_FORMAT_R8G8B8A8_UNORM));
  }

  void buildGraph() override {
    shader_pipelines_.clear();
    auto &bufferA = graph->feedback(
        "buffer.a", {},
        feedbackDesc("shadertoy.buffer.a", "buffer_a.frag", 0, {}));

    auto &bufferB = graph->feedback(
        "buffer.b", {bufferA},
        feedbackDesc("shadertoy.buffer.b", "buffer_b.frag", 1, {0}));

    auto &bufferC = graph->feedback(
        "buffer.c", {bufferA, bufferB},
        feedbackDesc("shadertoy.buffer.c", "buffer_c.frag", 2, {0, 1}));

    auto &bufferD = graph->feedback(
        "buffer.d", {bufferA, bufferB, bufferC},
        feedbackDesc("shadertoy.buffer.d", "buffer_d.frag", 3, {0, 1, 2}));

    auto &imagePass = graph->fullscreen(
        "image", {bufferA, bufferB, bufferC, bufferD}, imageDesc({0, 1, 2, 3}));
    bindChannels(bufferA, 0, {});
    bindChannels(bufferB, 1, {0});
    bindChannels(bufferC, 2, {0, 1});
    bindChannels(bufferD, 3, {0, 1, 2});
    bindChannels(imagePass, std::nullopt, {0, 1, 2, 3});
    graph->present(imagePass);
    for (const auto &pass : graph->passes()) {
      const auto capability =
          pass.get().capability<vkr::exec::GraphicsPipelineCapability>();
      if (capability) {
        shader_pipelines_.emplace_back(capability->get());
      }
    }
  }

  void onDraw() override {
    auto &shadertoyUBO = scene->uniformBuffer<UniformBufferShaderToyObject>(
        kShaderToyUniformName);

    const uint64_t revision = shadertoyPipelineRevision();
    if (revision != shadertoy_pipeline_revision_) {
      shadertoy_pipeline_revision_ = revision;
      shadertoy_frame_offset_ = timer->frameCount();
      shadertoy_time_offset_ = timer->elapsedTime();
    }

    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);
    const uint64_t frameCount = timer->frameCount();
    const uint64_t shadertoyFrame = frameCount >= shadertoy_frame_offset_
                                        ? frameCount - shadertoy_frame_offset_
                                        : 0;
    const float shadertoyTime = timer->elapsedTime() - shadertoy_time_offset_;

    UniformBufferShaderToyObject ubo{};
    ubo.iResolution = glm::vec3(static_cast<float>(ctx.window.width),
                                static_cast<float>(ctx.window.height),
                                static_cast<float>(ctx.window.ratio()));
    ubo.iTime = shadertoyTime;
    ubo.iTimeDelta = timer->deltaTime();
    ubo.iFrameRate = timer->fps();
    ubo.iFrame = static_cast<int>(shadertoyFrame);
    ubo.iMouse = isViewportMouseActive() ? shadertoyMouse() : glm::vec4{0.0f};
    ubo.iDate = glm::vec4(static_cast<float>(now->tm_year + 1900),
                          static_cast<float>(now->tm_mon),
                          static_cast<float>(now->tm_mday),
                          static_cast<float>(now->tm_hour * 3600 +
                                             now->tm_min * 60 + now->tm_sec));
    ubo.iChannelTime = glm::vec4(shadertoyTime);

    const glm::vec4 channelResolution{
        static_cast<float>(ctx.window.width),
        static_cast<float>(ctx.window.height),
        1.0f,
        0.0f,
    };
    for (auto &resolution : ubo.iChannelResolution) {
      resolution = channelResolution;
    }

    shadertoyUBO.update(executor->frameIndex(), ubo);
  }

  void configure() override {
    ctx = vkr::exec::RenderAppDesc::windowed("shadertoy", "ShaderToy Viewer");
    ctx.swapchain = {
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    };

    ctx.camera = {
        .locked = true,
    };

    ctx.ui.viewportFlipY = true;
  }
};

VKR_APP_RUN(ShaderToyApp)
