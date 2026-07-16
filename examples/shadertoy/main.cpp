#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class ShaderToyApp : public vkr::VulkanApplication {
private:
  glm::vec4 shadertoyMouse() const {
    const auto mousePosition = viewportMousePosition();
    const bool mouseDown =
        inputTracer->isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);

    return {mousePosition.x, mousePosition.y, mouseDown ? mousePosition.x : 0.0f,
            mouseDown ? mousePosition.y : 0.0f};
  }

  [[nodiscard]] auto shadertoyTargetDesc() const
      -> vkr::resource::OffscreenTargetDesc {
    return {.color = {.width = swapchain->width(),
                      .height = swapchain->height(),
                      .format = VK_FORMAT_R8G8B8A8_UNORM,
                      .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               VK_IMAGE_USAGE_SAMPLED_BIT,
                      .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      .createSampler = true}};
  }

  [[nodiscard]] auto shadertoyDescriptorBindings() const
      -> std::vector<vkr::pipeline::DescriptorBinding> {
    return {{.name = "shadertoy",
             .layout = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                        VK_SHADER_STAGE_FRAGMENT_BIT}}};
  }

  [[nodiscard]] auto shadertoyDescriptorPool(uint32_t imageSamplerCount) const
      -> vkr::pipeline::DescriptorPoolDesc {
    return {.poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                           vkr::core::MAX_FRAMES_IN_FLIGHT},
                          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                           vkr::core::MAX_FRAMES_IN_FLIGHT *
                               imageSamplerCount}},
            .maxSets = vkr::core::MAX_FRAMES_IN_FLIGHT};
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
            vkr::resource::ShaderModuleDesc::fragmentGlslFile(
                assetSystem->resolveApp("shaders/shadertoy/" + fragmentShader)
                    .string())),
    };
    pipeline.depthStencil =
        vkr::pipeline::GraphicsDepthStencilDesc::disabled();
    pipeline.rasterization =
        vkr::pipeline::GraphicsRasterizationDesc::noCull();
    return pipeline;
  }

  [[nodiscard]] auto feedbackDesc(const std::string &name,
                                  const std::string &fragmentShader,
                                  uint32_t sourceCount)
      -> vkr::render::FeedbackFullscreenPassDesc {
    vkr::render::FeedbackFullscreenPassDesc desc{};
    desc.target = {.target = shadertoyTargetDesc()};
    desc.descriptorBindings = shadertoyDescriptorBindings();
    desc.descriptorPool = shadertoyDescriptorPool(sourceCount + 1);
    desc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}};
    desc.historyInput = vkr::render::FullscreenPassInputDesc{.binding = 1};

    desc.inputs.reserve(sourceCount);
    for (uint32_t index = 0; index < sourceCount; ++index) {
      desc.inputs.push_back(
          vkr::render::FullscreenPassInputDesc{.binding = 2 + index});
    }

    desc.pipeline = shadertoyPipeline(name, fragmentShader);
    return desc;
  }

  [[nodiscard]] auto imageDesc(uint32_t sourceCount)
      -> vkr::render::FullscreenPassDesc {
    vkr::render::FullscreenPassDesc desc{};
    desc.target = shadertoyTargetDesc();
    desc.descriptorBindings = shadertoyDescriptorBindings();
    desc.descriptorPool = shadertoyDescriptorPool(sourceCount);
    desc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}};

    desc.inputs.reserve(sourceCount);
    for (uint32_t index = 0; index < sourceCount; ++index) {
      desc.inputs.push_back(
          vkr::render::FullscreenPassInputDesc{.binding = 1 + index});
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

  void createResources() override {
    resourceManager
        ->createUniformBuffer<vkr::resource::UniformBufferShaderToyObject>(
            "shadertoy", {});
  }

  void buildRenderGraph() override {
    auto &bufferA =
        renderGraph->addPass<vkr::render::FeedbackFullscreenPass>(
            *renderer, *device, *commandPool, *resourceManager);
    bufferA.setName("buffer.a").read("buffer.a.history").write("buffer.a");
    bufferA.update(feedbackDesc("shadertoy.buffer.a", "buffer_a.frag", 0));

    auto &bufferB =
        renderGraph->addPass<vkr::render::FeedbackFullscreenPass>(
            *renderer, *device, *commandPool, *resourceManager,
            std::vector<vkr::render::FullscreenPassSource>{
                vkr::render::FullscreenPassSource{bufferA}});
    bufferB.setName("buffer.b")
        .read("buffer.b.history")
        .read("buffer.a")
        .write("buffer.b");
    bufferB.update(feedbackDesc("shadertoy.buffer.b", "buffer_b.frag", 1));

    auto &bufferC =
        renderGraph->addPass<vkr::render::FeedbackFullscreenPass>(
            *renderer, *device, *commandPool, *resourceManager,
            std::vector<vkr::render::FullscreenPassSource>{
                vkr::render::FullscreenPassSource{bufferA},
                vkr::render::FullscreenPassSource{bufferB}});
    bufferC.setName("buffer.c")
        .read("buffer.c.history")
        .read("buffer.a")
        .read("buffer.b")
        .write("buffer.c");
    bufferC.update(feedbackDesc("shadertoy.buffer.c", "buffer_c.frag", 2));

    auto &bufferD =
        renderGraph->addPass<vkr::render::FeedbackFullscreenPass>(
            *renderer, *device, *commandPool, *resourceManager,
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
    bufferD.update(feedbackDesc("shadertoy.buffer.d", "buffer_d.frag", 3));

    auto &imagePass = renderGraph->addPass<vkr::render::FullscreenPass>(
        *renderer, *device, *commandPool, *resourceManager,
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
    imagePass.update(imageDesc(4));

    vkr::render::UiPassDesc uiDesc{};
    uiDesc.layoutMode = ctx.ui.layoutMode;
    uiDesc.descriptorPool = {
        .poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
                      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16}},
        .maxSets = vkr::core::MAX_FRAMES_IN_FLIGHT};
    uiDesc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}};

    auto &uiPass = renderGraph->addPass<vkr::render::UiPass>(
        *renderer, *window, *instance, *surface, *device, *commandPool,
        *swapchain, *resourceManager, *assetSystem, ctx.camera,
        vkr::render::FullscreenPassSource{imagePass}, *renderGraph, *timer,
        ctx.ui);
    uiPass.setName("ui").read("scene.color").write("swapchain");
    uiPass.update(uiDesc);

    auto &presentPass =
        renderGraph->addPass<vkr::render::PresentPass>(*renderer);
    presentPass.setName("present");
  }

  void onDraw() override {
    auto shadertoyUBO = resourceManager->getUniformBuffer("shadertoy");
    if (!shadertoyUBO) {
      return;
    }

    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);

    vkr::resource::UniformBufferShaderToyObject ubo{};
    ubo.iResolution = glm::vec3(static_cast<float>(ctx.window.width),
                                static_cast<float>(ctx.window.height),
                                static_cast<float>(ctx.window.ratio()));
    ubo.iTime = timer->elapsedTime();
    ubo.iTimeDelta = timer->deltaTime();
    ubo.iFrameRate = timer->fps();
    ubo.iFrame = static_cast<int>(timer->frameCount());
    ubo.iMouse = isViewportMouseActive() ? shadertoyMouse() : glm::vec4{0.0f};
    ubo.iDate = glm::vec4(static_cast<float>(now->tm_year + 1900),
                          static_cast<float>(now->tm_mon),
                          static_cast<float>(now->tm_mday),
                          static_cast<float>(now->tm_hour * 3600 +
                                             now->tm_min * 60 + now->tm_sec));
    ubo.iChannelTime = glm::vec4(timer->elapsedTime());

    const glm::vec3 channelResolution{
        static_cast<float>(ctx.window.width),
        static_cast<float>(ctx.window.height),
        1.0f,
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
    };

    ctx.swapchain = {
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    };

    ctx.camera = {
        .locked = true,
    };
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
