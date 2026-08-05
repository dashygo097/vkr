#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

namespace {

struct UniformBuffer3DObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

} // namespace

class TeapotApp : public vkr::exec::RenderApplication {
private:
  void createResources() override {
    vkr::scene::Mesh<vkr::scene::VertexNormalTexture3D> teapot(*device,
                                                               *commandPool);
    teapot.load(assetSystem->resolve("objects/teapot/teapot.obj"));

    scene->createMesh("teapot", teapot);
    scene->createTexture(
        "teapot_texture",
        assetSystem->resolve("objects/teapot/default.png").string());

    scene->createUniformBuffer<UniformBuffer3DObject>("default", {});
  }

  void buildGraph() override {
    vkr::exec::RasterPassDesc desc{};
    desc.color(swapchain->width(), swapchain->height(),
               VK_FORMAT_R8G8B8A8_UNORM)
        .sampledColor()
        .depth(VK_FORMAT_D32_SFLOAT)
        .uniform(0, "default", VK_SHADER_STAGE_VERTEX_BIT)
        .texture(1, "teapot_texture", VK_SHADER_STAGE_FRAGMENT_BIT)
        .pipeline("teapot-local")
        .vertexInput(vkr::scene::VertexNormalTexture3D::vertexInputDesc())
        .vertexShader(vkr::resource::ShaderModuleDesc::vertexGlslFile(
            assetSystem->resolve("shaders/teapot/teapot.vert").string()))
        .fragmentShader(vkr::resource::ShaderModuleDesc::fragmentGlslFile(
            assetSystem->resolve("shaders/teapot/teapot.frag").string()))
        .depthTest()
        .noCull()
        .clearColor(0.0f, 0.0f, 0.0f, 1.0f)
        .clearDepth();

    auto &rasterPass = graph->addPass<vkr::exec::RasterPass>(
        *executor, *device, *commandPool, *scene);
    rasterPass.setName("raster").write("scene.raw");
    rasterPass.update(desc);

    vkr::exec::FullscreenPassDesc postDesc{};
    postDesc
        .color(swapchain->width(), swapchain->height(),
               VK_FORMAT_R8G8B8A8_UNORM)
        .sampledColor()
        .clearColor(0.0f, 0.0f, 0.0f, 1.0f)
        .pipeline("postprocess")
        .vertexInput(vkr::scene::VertexInputDesc::none())
        .vertexShader(vkr::resource::ShaderModuleDesc::vertexGlslFile(
            assetSystem->resolve("shaders/postprocess/postprocess.vert")
                .string()))
        .fragmentShader(vkr::resource::ShaderModuleDesc::fragmentGlslFile(
            assetSystem->resolve("shaders/postprocess/postprocess.frag")
                .string()))
        .disableDepthTest()
        .noCull();

    auto &postProcessPass = graph->addPass<vkr::exec::PostProcessPass>(
        *executor, *device, *commandPool,
        vkr::exec::RenderPassSource{rasterPass});
    postProcessPass.setName("postprocess")
        .read("scene.raw")
        .write("scene.color");
    postProcessPass.update(postDesc);

    auto &uiPass = graph->addPass<vkr::exec::UiPass>(
        *executor, *window, *instance, *surface, *device, *commandPool,
        *commandBuffers, *swapchain, *scene, *assetSystem, ctx.camera,
        vkr::exec::RenderPassSource{postProcessPass}, *graph, *timer, ctx.ui);
    uiPass.setName("ui").read("scene.color").write("swapchain");

    auto &presentPass = graph->addPass<vkr::exec::PresentPass>(*executor);
    presentPass.setName("present");
  }

  void onDraw() override {
    const uint32_t frameIndex = executor->frameIndex();

    UniformBuffer3DObject ubo{};
    ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.4f, -7.0f));
    ubo.model = glm::scale(ubo.model, glm::vec3(0.04f));
    ubo.view = camera->getView();
    ubo.proj = camera->getProjection();

    scene->getUniformBuffer("default")->updateRaw(frameIndex, &ubo,
                                                  sizeof(ubo));

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
        .title = "Teapot",
        .width = 1200,
        .height = 900,
    };

    ctx.instance = {
        .name = "teapot",
        .version = VK_MAKE_VERSION(1, 0, 0),
        .surfaceIntegration = vkr::core::SurfaceIntegration::GLFW,
    };
    ctx.commandBuffers.size = 2;

    ctx.camera = {
        .movementSpeed = 5.0f,
        .mouseSensitivity = 0.5f,
        .aspectRatio = ctx.window.ratio(),
    };
  }
};

auto main() -> int {
  TeapotApp app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
