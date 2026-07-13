#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class ShaderToyApp : public vkr::VulkanApplication {
private:
  std::chrono::high_resolution_clock::time_point startTime;
  std::chrono::high_resolution_clock::time_point lastFrameTime;
  float totalTime{0.0f};
  int frameCount{0};

  glm::vec4 mouseState{0.0f};

  void createResources() override {
    resourceManager
        ->createUniformBuffer<vkr::resource::UniformBufferShaderToyObject>(
            "shadertoy", {});
  }

  void buildRenderGraph() override {
    vkr::render::RasterPassDesc desc{};
    desc.target = {
        .color = {.width = swapchain->width(),
                  .height = swapchain->height(),
                  .format = VK_FORMAT_R8G8B8A8_UNORM,
                  .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                           VK_IMAGE_USAGE_SAMPLED_BIT,
                  .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  .createSampler = true},
        .depth =
            vkr::resource::DepthAttachmentDesc{.width = swapchain->width(),
                                               .height = swapchain->height(),
                                               .format = VK_FORMAT_D32_SFLOAT}};
    desc.descriptorBindings = {
        {.name = "shadertoy",
         .layout = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                    VK_SHADER_STAGE_FRAGMENT_BIT}},
    };
    desc.descriptorPool = {
        .poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
                      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16}},
        .maxSets = vkr::core::MAX_FRAMES_IN_FLIGHT};
    desc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
                        VkClearValue{.depthStencil = {1.0f, 0}}};

    vkr::pipeline::GraphicsPipelineDesc shadertoy{};
    shadertoy.name = "shadertoy";
    shadertoy.vertexInput = vkr::resource::VertexInputDesc::none();

    shadertoy.shaders = {
        vkr::pipeline::GraphicsShaderStageDesc::vertex(
            vkr::resource::ShaderModuleDesc::vertexGlslFile(
                assetSystem->resolve("shaders/shadertoy/shadertoy.vert")
                    .string())),
        vkr::pipeline::GraphicsShaderStageDesc::fragment(
            vkr::resource::ShaderModuleDesc::fragmentGlslFile(
                assetSystem->resolve("shaders/shadertoy/shadertoy.frag")
                    .string())),
    };

    shadertoy.depthStencil =
        vkr::pipeline::GraphicsDepthStencilDesc::disabled();
    shadertoy.rasterization =
        vkr::pipeline::GraphicsRasterizationDesc::noCull();

    desc.pipeline = shadertoy;

    auto &rasterPass = renderGraph->addPass<vkr::render::RasterPass>(
        *renderer, *device, *commandPool, *resourceManager);
    rasterPass.setName("raster").write("scene.color");
    rasterPass.update(desc);

    addUiPass(rasterPass);

    auto &presentPass = renderGraph->addPass<vkr::render::PresentPass>();
    presentPass.setName("present").read("swapchain");
  }

  void onDraw() override {
    auto shadertoyUBO = resourceManager->getUniformBuffer("shadertoy");
    if (!shadertoyUBO) {
      return;
    }

    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime =
        std::chrono::duration<float, std::chrono::seconds::period>(
            currentTime - lastFrameTime)
            .count();
    totalTime = std::chrono::duration<float, std::chrono::seconds::period>(
                    currentTime - startTime)
                    .count();
    lastFrameTime = currentTime;

    const auto cursor = inputTracer->cursorPosition();

    if (inputTracer->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
      mouseState.z = static_cast<float>(cursor.x);
      mouseState.w = static_cast<float>(ctx.window.height - cursor.y);
    }

    mouseState.x = static_cast<float>(cursor.x);
    mouseState.y = static_cast<float>(ctx.window.height - cursor.y);

    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);

    vkr::resource::UniformBufferShaderToyObject ubo{};
    ubo.iResolution = glm::vec3(static_cast<float>(ctx.window.width),
                                static_cast<float>(ctx.window.height),
                                static_cast<float>(ctx.window.ratio()));
    ubo.iTime = totalTime;
    ubo.iTimeDelta = deltaTime;
    ubo.iFrameRate = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 60.0f;
    ubo.iFrame = frameCount++;
    ubo.iMouse = mouseState;
    ubo.iDate = glm::vec4(static_cast<float>(now->tm_year + 1900),
                          static_cast<float>(now->tm_mon),
                          static_cast<float>(now->tm_mday),
                          static_cast<float>(now->tm_hour * 3600 +
                                             now->tm_min * 60 + now->tm_sec));

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

    startTime = std::chrono::high_resolution_clock::now();
    lastFrameTime = startTime;
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
