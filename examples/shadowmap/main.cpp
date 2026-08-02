#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <vkr.hh>

namespace {

struct UniformBuffer3DObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct LightSpaceObject {
  alignas(16) glm::mat4 lightViewProj;
  alignas(16) glm::vec4 lightPosBias;
};

constexpr std::array<const char *, 4> ShadowSceneParts{
    "floor", "back_wall", "tall_block", "short_block"};

constexpr size_t shadowMapWidth = 480, shadowMapHeight = 640;

}; // namespace

class ShadowMappingApp : public vkr::exec::RenderApplication {
private:
  void createResources() override {
    for (const char *part : ShadowSceneParts) {
      vkr::scene::Mesh<vkr::scene::VertexNormal3D> mesh(*device, *commandPool);

      std::string path = "objects/shadow_scene/";
      path += part;
      path += ".obj";
      mesh.load(assetSystem->resolveApp(path).string());

      std::string meshName = "shadow_scene.";
      meshName += part;
      scene->createMesh(meshName, mesh);
    }

    scene->createUniformBuffer<UniformBuffer3DObject>("default", {});
    scene->createUniformBuffer<LightSpaceObject>("light", {});
  }

  void buildGraph() override {
    std::vector<std::string> meshNames{};
    meshNames.reserve(ShadowSceneParts.size());
    for (const char *part : ShadowSceneParts) {
      std::string meshName = "shadow_scene.";
      meshName += part;
      meshNames.push_back(std::move(meshName));
    }

    vkr::exec::RasterPassDesc shadowDesc{};
    shadowDesc
        .color(static_cast<uint32_t>(shadowMapWidth),
               static_cast<uint32_t>(shadowMapHeight), VK_FORMAT_R8G8B8A8_UNORM)
        .colorUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        .depth(static_cast<uint32_t>(shadowMapWidth),
               static_cast<uint32_t>(shadowMapHeight), VK_FORMAT_D32_SFLOAT)
        .sampledDepth()
        .uniform(0, "default", VK_SHADER_STAGE_VERTEX_BIT)
        .uniform(1, "light", VK_SHADER_STAGE_VERTEX_BIT)
        .pipeline("shadowmap")
        .vertexInput(vkr::scene::VertexNormal3D::vertexInputDesc())
        .vertexShader(vkr::resource::ShaderModuleDesc::vertexGlslFile(
            assetSystem->resolveApp("shaders/shadowmap/shadow.vert").string()))
        .fragmentShader(vkr::resource::ShaderModuleDesc::fragmentGlslFile(
            assetSystem->resolveApp("shaders/shadowmap/shadow.frag").string()))
        .depthTest()
        .noCull()
        .meshes(meshNames)
        .clearColor(1.0f, 1.0f, 1.0f, 1.0f)
        .clearDepth();

    auto &shadowPass = graph->addPass<vkr::exec::RasterPass>(
        *executor, *device, *commandPool, *scene);
    shadowPass.setName("shadowmap").write("shadow.depth");
    shadowPass.update(shadowDesc);

    vkr::exec::RasterPassDesc cameraDesc{};
    cameraDesc
        .color(swapchain->width(), swapchain->height(),
               VK_FORMAT_R8G8B8A8_UNORM)
        .sampledColor()
        .depth(VK_FORMAT_D32_SFLOAT)
        .uniform(0, "default", VK_SHADER_STAGE_VERTEX_BIT)
        .uniform(1, "light",
                 VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
        .inputDepth(2, VK_SHADER_STAGE_FRAGMENT_BIT)
        .pipeline("shadowmap-camera")
        .vertexInput(vkr::scene::VertexNormal3D::vertexInputDesc())
        .vertexShader(vkr::resource::ShaderModuleDesc::vertexGlslFile(
            assetSystem->resolveApp("shaders/shadowmap/camera.vert").string()))
        .fragmentShader(vkr::resource::ShaderModuleDesc::fragmentGlslFile(
            assetSystem->resolveApp("shaders/shadowmap/camera.frag").string()))
        .depthTest()
        .noCull()
        .meshes(std::move(meshNames))
        .clearColor(0.03f, 0.035f, 0.04f, 1.0f)
        .clearDepth();

    auto &cameraPass = graph->addPass<vkr::exec::RasterPass>(
        *executor, *device, *commandPool, *scene);
    cameraPass.addSource(vkr::exec::RenderPassSource{shadowPass});
    cameraPass.setName("camera").read("shadow.depth").write("scene.color");
    cameraPass.update(cameraDesc);

    auto &uiPass = graph->addPass<vkr::exec::UiPass>(
        *executor, *window, *instance, *surface, *device, *commandPool,
        *commandBuffers, *swapchain, *scene, *assetSystem, ctx.camera,
        vkr::exec::RenderPassSource{cameraPass}, *graph, *timer, ctx.ui);
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

    scene->getUniformBuffer("default")->updateRaw(frameIndex, &ubo,
                                                  sizeof(ubo));

    const glm::vec3 lightPos{-2.5f, 5.0f, 2.5f};
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3{0.0f, 0.8f, 0.0f},
                                      glm::vec3{0.0f, 1.0f, 0.0f});
    glm::mat4 lightProj = glm::ortho(-9.0f, 9.0f, -9.0f, 9.0f, 0.1f, 12.0f);
    lightProj[1][1] *= -1.0f;

    LightSpaceObject light{};
    light.lightViewProj = lightProj * lightView;
    light.lightPosBias = glm::vec4(lightPos, 0.003f);

    scene->getUniformBuffer("light")->updateRaw(frameIndex, &light,
                                                sizeof(light));

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
        .title = "Shadow Mapping",
        .width = 1200,
        .height = 900,
    };

    ctx.instance = {
        .name = "shadow mapping",
        .version = VK_MAKE_VERSION(1, 0, 0),
        .surfaceIntegration = vkr::core::SurfaceIntegration::GLFW,
    };
    ctx.commandBuffers.size = 2;

    ctx.camera = {
        .movementSpeed = 5.0f,
        .mouseSensitivity = 0.5f,
        .fov = 45.0f,
        .aspectRatio = ctx.window.ratio(),
    };
  }
};

auto main() -> int {
  ShadowMappingApp app;

  try {
    app.run();
  } catch (const std::exception e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
