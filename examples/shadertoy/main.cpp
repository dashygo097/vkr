#include <cstdint>
#include <ctime>
#include <glm/glm.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <vkr.hh>
#include <vkr/util/io.hh>
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

class ShaderToyApp : public vkr::VulkanApplication {
private:
  static constexpr uint32_t kShaderToyChannelCount = 4;
  static constexpr std::string_view kShaderToyUniformName{"shadertoy"};
  static constexpr std::string_view kFallbackTextureName{"shadertoy.black"};

  uint64_t shadertoy_pipeline_revision_{0};
  uint64_t shadertoy_frame_offset_{0};
  float shadertoy_time_offset_{0.0f};

  glm::vec4 shadertoyMouse() const {
    const auto mousePosition = viewportMousePosition();
    const bool mouseDown =
        inputTracer->isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);

    return {mousePosition.x, mousePosition.y,
            mouseDown ? mousePosition.x : 0.0f,
            mouseDown ? mousePosition.y : 0.0f};
  }

  [[nodiscard]] auto shadertoyTargetDesc() const
      -> vkr::render::OffscreenTargetDesc {
    return {.color = {.width = swapchain->width(),
                      .height = swapchain->height(),
                      .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                      .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               VK_IMAGE_USAGE_SAMPLED_BIT,
                      .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      .createSampler = true}};
  }

  [[nodiscard]] auto shadertoyDescriptorBindings() const
      -> std::vector<vkr::pipeline::DescriptorBinding> {
    return {{.name = std::string(kShaderToyUniformName),
             .layout = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                        VK_SHADER_STAGE_FRAGMENT_BIT}}};
  }

  [[nodiscard]] auto shadertoyDescriptorBindings(
      const std::vector<uint32_t> &fallbackChannels) const
      -> std::vector<vkr::pipeline::DescriptorBinding> {
    auto bindings = shadertoyDescriptorBindings();

    for (uint32_t channel : fallbackChannels) {
      bindings.push_back({.name = std::string(kFallbackTextureName),
                          .layout = {channelInput(channel).binding,
                                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                     1, VK_SHADER_STAGE_FRAGMENT_BIT}});
    }

    return bindings;
  }

  [[nodiscard]] auto shadertoyDescriptorPool(uint32_t imageSamplerCount) const
      -> vkr::pipeline::DescriptorPoolDesc {
    vkr::pipeline::DescriptorPoolDesc desc{
        .poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                       vkr::core::MAX_FRAMES_IN_FLIGHT}},
        .maxSets = vkr::core::MAX_FRAMES_IN_FLIGHT};

    if (imageSamplerCount > 0) {
      desc.poolSizes.push_back(
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
           vkr::core::MAX_FRAMES_IN_FLIGHT * imageSamplerCount});
    }

    return desc;
  }

  [[nodiscard]] static auto channelInput(uint32_t channel)
      -> vkr::render::FullscreenPassInputDesc {
    return vkr::render::FullscreenPassInputDesc::image(1U + channel);
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

  [[nodiscard]] auto shadertoyFragmentSource(const std::string &fragmentShader,
                                             const std::string &label) const
      -> vkr::resource::ShaderModuleDesc {
    const auto path =
        assetSystem->resolveApp("shaders/shadertoy/" + fragmentShader);
    const std::string body = vkr::util::fread_string(path.string());

    std::string source{};
    source.reserve(body.size() + 2048);
    source += "#version 450\n"
              "\n"
              "layout(binding = 0) uniform ShaderToyUBO {\n"
              "  vec3 iResolution;\n"
              "  float iTime;\n"
              "  float iTimeDelta;\n"
              "  float iFrameRate;\n"
              "  int iFrame;\n"
              "  vec4 iMouse;\n"
              "  vec4 iDate;\n"
              "  vec4 iChannelTime;\n"
              "  vec3 iChannelResolution[4];\n"
              "};\n"
              "\n"
              "layout(binding = 1) uniform sampler2D iChannel0;\n"
              "layout(binding = 2) uniform sampler2D iChannel1;\n"
              "layout(binding = 3) uniform sampler2D iChannel2;\n"
              "layout(binding = 4) uniform sampler2D iChannel3;\n"
              "\n"
              "layout(location = 0) in vec2 fragUV;\n"
              "layout(location = 0) out vec4 outColor;\n"
              "\n"
              "#define texture2D texture\n"
              "#define textureCube texture\n"
              "\n";
    source += body;
    source += "\n"
              "void main() {\n"
              "  mainImage(outColor, fragUV * iResolution.xy);\n"
              "}\n";

    return vkr::resource::ShaderModuleDesc::fragmentGlslSource(source, label);
  }

  [[nodiscard]] auto shadertoyPipeline(const std::string &name,
                                       const std::string &fragmentShader) const
      -> vkr::pipeline::GraphicsPipelineDesc {
    vkr::pipeline::GraphicsPipelineDesc pipeline{};
    pipeline.name = name;
    pipeline.vertexInput = vkr::resource::VertexInputDesc::none();
    pipeline.shaders = {
        vkr::pipeline::GraphicsShaderStageDesc::vertex(
            vkr::resource::ShaderModuleDesc::vertexGlslFile(
                assetSystem->resolveApp("shaders/shadertoy/shadertoy.vert")
                    .string())),
        vkr::pipeline::GraphicsShaderStageDesc::fragment(
            shadertoyFragmentSource(fragmentShader, name + ".frag")),
    };
    pipeline.depthStencil = vkr::pipeline::GraphicsDepthStencilDesc::disabled();
    pipeline.rasterization = vkr::pipeline::GraphicsRasterizationDesc::noCull();
    return pipeline;
  }

  [[nodiscard]] auto feedbackDesc(const std::string &name,
                                  const std::string &fragmentShader,
                                  std::optional<uint32_t> historyChannel,
                                  const std::vector<uint32_t> &sourceChannels)
      -> vkr::render::FeedbackFullscreenPassDesc {
    const auto fallback = fallbackChannels(historyChannel, sourceChannels);

    vkr::render::FeedbackFullscreenPassDesc desc{};
    desc.target = {.target = shadertoyTargetDesc()};
    desc.descriptorBindings = shadertoyDescriptorBindings(fallback);
    desc.descriptorPool = shadertoyDescriptorPool(static_cast<uint32_t>(
        sourceChannels.size() + fallback.size() + (historyChannel ? 1U : 0U)));
    desc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}};
    if (historyChannel) {
      desc.historyInput = channelInput(*historyChannel);
    }

    desc.inputs.reserve(sourceChannels.size());
    for (uint32_t channel : sourceChannels) {
      desc.inputs.push_back(channelInput(channel));
    }

    desc.pipeline = shadertoyPipeline(name, fragmentShader);
    return desc;
  }

  [[nodiscard]] auto imageDesc(const std::vector<uint32_t> &sourceChannels)
      -> vkr::render::FullscreenPassDesc {
    const auto fallback = fallbackChannels(std::nullopt, sourceChannels);

    vkr::render::FullscreenPassDesc desc{};
    desc.target = shadertoyTargetDesc();
    desc.descriptorBindings = shadertoyDescriptorBindings(fallback);
    desc.descriptorPool = shadertoyDescriptorPool(
        static_cast<uint32_t>(sourceChannels.size() + fallback.size()));
    desc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}};

    desc.inputs.reserve(sourceChannels.size());
    for (uint32_t channel : sourceChannels) {
      desc.inputs.push_back(channelInput(channel));
    }

    desc.pipeline = shadertoyPipeline("shadertoy.image", "image.frag");
    return desc;
  }

  [[nodiscard]] auto viewportMousePosition() const -> glm::vec2 {
    const auto cursor = inputTracer->cursorPosition();

    if (ctx.ui.layoutMode == vkr::ui::LayoutMode::FullScreen) {
      return {static_cast<float>(cursor.x),
              static_cast<float>(ctx.window.height - cursor.y)};
    }

    const auto viewport = ctx.ui.viewport;
    if (!ctx.ui.viewportFocused || viewport.width <= 0.0f ||
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
    if (ctx.ui.layoutMode == vkr::ui::LayoutMode::FullScreen) {
      return true;
    }

    const auto viewport = ctx.ui.viewport;
    return ctx.ui.viewportFocused && viewport.width > 0.0f &&
           viewport.height > 0.0f;
  }

  [[nodiscard]] auto shadertoyPipelineRevision() -> uint64_t {
    uint64_t hash = 1469598103934665603ULL;

    for (auto pass : renderGraph->passes()) {
      auto pipeline = pass.get().editablePipeline();
      if (!pipeline) {
        continue;
      }

      hash ^= pipeline->get().revision() + 0x9E3779B97F4A7C15ULL +
              (hash << 6U) + (hash >> 2U);
    }

    return hash;
  }

  void resetShadertoyTimelineIfPipelineChanged() {
    const uint64_t revision = shadertoyPipelineRevision();
    if (revision == shadertoy_pipeline_revision_) {
      return;
    }

    shadertoy_pipeline_revision_ = revision;
    shadertoy_frame_offset_ = timer->frameCount();
    shadertoy_time_offset_ = timer->elapsedTime();
  }

  void createResources() override {
    scene->createUniformBuffer<UniformBufferShaderToyObject>(
        std::string(kShaderToyUniformName), {});
    scene->createTexture(
        std::string(kFallbackTextureName),
        vkr::scene::TextureDesc::sampled2D(1, 1,
                                           VK_FORMAT_R8G8B8A8_UNORM));
  }

  void buildRenderGraph() override {
    auto &bufferA = renderGraph->addPass<vkr::render::FeedbackFullscreenPass>(
        *renderer, *device, *graphicsCommandPool, *scene);
    bufferA.setName("buffer.a").read("buffer.a.history").write("buffer.a");
    bufferA.update(feedbackDesc("shadertoy.buffer.a", "buffer_a.frag", 0, {}));

    auto &bufferB = renderGraph->addPass<vkr::render::FeedbackFullscreenPass>(
        *renderer, *device, *graphicsCommandPool, *scene,
        std::vector<vkr::render::FullscreenPassSource>{
            vkr::render::FullscreenPassSource{bufferA}});
    bufferB.setName("buffer.b")
        .read("buffer.b.history")
        .read("buffer.a")
        .write("buffer.b");
    bufferB.update(feedbackDesc("shadertoy.buffer.b", "buffer_b.frag", 1, {0}));

    auto &bufferC = renderGraph->addPass<vkr::render::FeedbackFullscreenPass>(
        *renderer, *device, *graphicsCommandPool, *scene,
        std::vector<vkr::render::FullscreenPassSource>{
            vkr::render::FullscreenPassSource{bufferA},
            vkr::render::FullscreenPassSource{bufferB}});
    bufferC.setName("buffer.c")
        .read("buffer.c.history")
        .read("buffer.a")
        .read("buffer.b")
        .write("buffer.c");
    bufferC.update(
        feedbackDesc("shadertoy.buffer.c", "buffer_c.frag", 2, {0, 1}));

    auto &bufferD = renderGraph->addPass<vkr::render::FeedbackFullscreenPass>(
        *renderer, *device, *graphicsCommandPool, *scene,
        std::vector<vkr::render::FullscreenPassSource>{
            vkr::render::FullscreenPassSource{bufferA},
            vkr::render::FullscreenPassSource{bufferB},
            vkr::render::FullscreenPassSource{bufferC}});
    bufferD.setName("buffer.d")
        .read("buffer.d.history")
        .read("buffer.a")
        .read("buffer.b")
        .read("buffer.c")
        .write("buffer.d");
    bufferD.update(
        feedbackDesc("shadertoy.buffer.d", "buffer_d.frag", 3, {0, 1, 2}));

    auto &imagePass = renderGraph->addPass<vkr::render::FullscreenPass>(
        *renderer, *device, *graphicsCommandPool, *scene,
        std::vector<vkr::render::FullscreenPassSource>{
            vkr::render::FullscreenPassSource{bufferA},
            vkr::render::FullscreenPassSource{bufferB},
            vkr::render::FullscreenPassSource{bufferC},
            vkr::render::FullscreenPassSource{bufferD}});
    imagePass.setName("image")
        .read("buffer.a")
        .read("buffer.b")
        .read("buffer.c")
        .read("buffer.d")
        .write("scene.color");
    imagePass.update(imageDesc({0, 1, 2, 3}));

    vkr::render::UiPassDesc uiDesc{};
    uiDesc.layoutMode = ctx.ui.layoutMode;
    uiDesc.descriptorPool = {
        .poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
                      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16}},
        .maxSets = vkr::core::MAX_FRAMES_IN_FLIGHT};
    uiDesc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}};

    auto &uiPass = renderGraph->addPass<vkr::render::UiPass>(
        *renderer, *window, *instance, *surface, *device, *graphicsCommandPool,
        *swapchain, *scene, *assetSystem, ctx.camera,
        vkr::render::FullscreenPassSource{imagePass}, *renderGraph, *timer,
        ctx.ui);
    uiPass.setName("ui").read("scene.color").write("swapchain");
    uiPass.update(uiDesc);

    auto &presentPass =
        renderGraph->addPass<vkr::render::PresentPass>(*renderer);
    presentPass.setName("present");
  }

  void onDraw() override {
    auto shadertoyUBO =
        scene->getUniformBuffer(std::string(kShaderToyUniformName));
    if (!shadertoyUBO) {
      return;
    }

    resetShadertoyTimelineIfPipelineChanged();

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

    shadertoyUBO->updateRaw(renderer->frameIndex(), &ubo, sizeof(ubo));
  }

  void configure() override {
    ctx.window = {
        .title = "ShaderToy Viewer",
        .width = 1200,
        .height = 900,
    };

    ctx.instance = {
        .name = "shadertoy",
        .version = VK_MAKE_VERSION(1, 0, 0),
        .surfaceIntegration = vkr::core::SurfaceIntegration::GLFW,
    };

    ctx.swapchain = {
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    };

    ctx.camera = {
        .locked = true,
    };

    ctx.ui.viewportFlipY = true;
  }
};

auto main(int argc, char *argv[]) -> int {
  ShaderToyApp app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
