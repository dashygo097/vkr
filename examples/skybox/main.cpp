#include <array>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <vkr.hh>
#include <vulkan/vulkan.h>

namespace {

struct UniformBuffer3DObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

constexpr std::array<const char *, 6> CornellBoxParts{
    "floor", "left", "light", "right", "shortbox", "tallbox"};

} // namespace

class SkyboxApp : public vkr::exec::RenderApplication {
private:
  void createResources() override {
    scene->createCubemap("skybox", skyboxFaces(), VK_FORMAT_R8G8B8A8_SRGB);

    vkr::scene::Mesh<vkr::scene::VertexSkybox3D> skybox(*device,
                                                        *graphicsCommandPool);
    skybox.load(vkr::scene::skyboxCubeVertices(),
                vkr::scene::skyboxCubeIndices());
    scene->createMesh("skybox", skybox);
    scene->createUniformBuffer<UniformBuffer3DObject>("skybox", {});

    for (const char *part : CornellBoxParts) {
      vkr::scene::Mesh<vkr::scene::Vertex3D> cornellPart(*device,
                                                         *graphicsCommandPool);

      std::string path = "objects/cornellbox/";
      path += part;
      path += ".obj";
      cornellPart.load(assetSystem->resolveApp(path).string());

      std::string meshName = "cornellbox.";
      meshName += part;
      scene->createMesh(meshName, cornellPart);
    }

    scene->createUniformBuffer<UniformBuffer3DObject>("cornellbox", {});
  }

  void buildGraph() override {
    vkr::exec::RasterPassDesc skyboxDesc{};
    skyboxDesc
        .color(swapchain->width(), swapchain->height(),
               VK_FORMAT_R8G8B8A8_UNORM)
        .sampledColor()
        .depth(VK_FORMAT_D32_SFLOAT)
        .uniform(0, "skybox", VK_SHADER_STAGE_VERTEX_BIT)
        .cubemap(1, "skybox", VK_SHADER_STAGE_FRAGMENT_BIT)
        .mesh("skybox")
        .pipeline("skybox")
        .vertexInput(vkr::scene::VertexSkybox3D::vertexInputDesc())
        .vertexShader(vkr::resource::ShaderModuleDesc::vertexGlslFile(
            assetSystem->resolveApp("shaders/skybox/skybox.vert").string()))
        .fragmentShader(vkr::resource::ShaderModuleDesc::fragmentGlslFile(
            assetSystem->resolveApp("shaders/skybox/skybox.frag").string()))
        .readOnlyDepthTest()
        .noCull()
        .clearColor(0.0f, 0.0f, 0.0f, 1.0f)
        .clearDepth();

    auto &skyboxPass = graph->addPass<vkr::exec::RasterPass>(
        *executor, *device, *graphicsCommandPool, *scene);
    skyboxPass.setName("skybox").write("scene.skybox");
    skyboxPass.update(skyboxDesc);

    vkr::exec::RasterPassDesc cornellDesc{};
    cornellDesc
        .color(swapchain->width(), swapchain->height(),
               VK_FORMAT_R8G8B8A8_UNORM)
        .sampledColor()
        .depth(VK_FORMAT_D32_SFLOAT)
        .uniform(0, "cornellbox", VK_SHADER_STAGE_VERTEX_BIT)
        .pipeline("cornellbox")
        .vertexInput(vkr::scene::Vertex3D::vertexInputDesc())
        .vertexShader(vkr::resource::ShaderModuleDesc::vertexGlslFile(
            assetSystem->resolveApp("shaders/cornell/cornell.vert").string()))
        .fragmentShader(vkr::resource::ShaderModuleDesc::fragmentGlslFile(
            assetSystem->resolveApp("shaders/cornell/cornell.frag").string()))
        .depthTest()
        .noCull()
        .clearColor(0.0f, 0.0f, 0.0f, 0.0f)
        .clearDepth();

    for (const char *part : CornellBoxParts) {
      std::string meshName = "cornellbox.";
      meshName += part;
      cornellDesc.mesh(meshName);
    }

    auto &cornellPass = graph->addPass<vkr::exec::RasterPass>(
        *executor, *device, *graphicsCommandPool, *scene);
    cornellPass.setName("cornellbox").write("scene.cornell");
    cornellPass.update(cornellDesc);

    vkr::exec::FullscreenPassDesc compositeDesc{};
    compositeDesc.target = {
        .color = {.width = swapchain->width(),
                  .height = swapchain->height(),
                  .format = VK_FORMAT_R8G8B8A8_UNORM,
                  .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                           VK_IMAGE_USAGE_SAMPLED_BIT,
                  .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  .createSampler = true}};
    compositeDesc.clearValues = {
        VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}};
    compositeDesc.pipeline.setName("skybox-cornell-composite")
        .vertexInputDesc(vkr::scene::VertexInputDesc::none())
        .vertexShader(vkr::resource::ShaderModuleDesc::vertexGlslFile(
            assetSystem->resolveApp("shaders/composite/composite.vert")
                .string()))
        .fragmentShader(vkr::resource::ShaderModuleDesc::fragmentGlslFile(
            assetSystem->resolveApp("shaders/composite/composite.frag")
                .string()))
        .disableDepth()
        .noCull();

    auto &compositePass = graph->addPass<vkr::exec::CompositePass>(
        *executor, *device, *graphicsCommandPool,
        std::vector<vkr::exec::FullscreenPassSource>{
            vkr::exec::FullscreenPassSource{skyboxPass},
            vkr::exec::FullscreenPassSource{cornellPass}});
    compositePass.setName("composite")
        .read("scene.skybox")
        .read("scene.cornell")
        .write("scene.color");
    compositePass.update(compositeDesc);

    auto &uiPass = graph->addPass<vkr::exec::UiPass>(
        *executor, *window, *instance, *surface, *device, *graphicsCommandPool,
        *swapchain, *scene, *assetSystem, ctx.camera,
        vkr::exec::FullscreenPassSource{compositePass}, *graph, *timer, ctx.ui);
    uiPass.setName("ui").read("scene.color").write("swapchain");

    auto &presentPass = graph->addPass<vkr::exec::PresentPass>(*executor);
    presentPass.setName("present");
  }

  void onDraw() override {
    const uint32_t frameIndex = executor->frameIndex();

    UniformBuffer3DObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = camera->getView();
    ubo.proj = camera->getProjection();

    scene->getUniformBuffer("skybox")->updateRaw(frameIndex, &ubo, sizeof(ubo));
    scene->getUniformBuffer("cornellbox")
        ->updateRaw(frameIndex, &ubo, sizeof(ubo));

    if (ctx.ui.viewport.height > 0 &&
        ctx.ui.layoutMode == vkr::ui::LayoutMode::Standard) {
      ctx.camera.aspectRatio =
          ctx.ui.viewport.width / static_cast<float>(ctx.ui.viewport.height);
    } else {
      ctx.camera.aspectRatio = ctx.window.ratio();
    }
  }

  void configure() override {
    ctx.window = {
        .title = "Cornell Box",
        .width = 1200,
        .height = 900,
    };

    ctx.instance = {
        .name = "cornellbox",
        .version = VK_MAKE_VERSION(1, 0, 0),
        .surfaceIntegration = vkr::core::SurfaceIntegration::GLFW,
    };
    ctx.commandBuffers.size = 2;

    ctx.camera = {
        .movementSpeed = 5.0f,
        .mouseSensitivity = 0.5f,
        .fov = 50.0f,
        .aspectRatio = ctx.window.ratio(),
        .pos = {2.78f, 2.75f, -7.0f},
        .yaw = 90.0f,
    };
  }

  [[nodiscard]] auto skyboxFaces() const -> std::array<std::string, 6> {
    return {
        assetSystem->resolveApp("textures/skybox/right.ppm").string(),
        assetSystem->resolveApp("textures/skybox/left.ppm").string(),
        assetSystem->resolveApp("textures/skybox/top.ppm").string(),
        assetSystem->resolveApp("textures/skybox/bottom.ppm").string(),
        assetSystem->resolveApp("textures/skybox/front.ppm").string(),
        assetSystem->resolveApp("textures/skybox/back.ppm").string(),
    };
  }
};

auto main() -> int {
  SkyboxApp app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
