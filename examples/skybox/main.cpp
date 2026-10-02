#include <array>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <utility>
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

    scene->createMesh<vkr::scene::VertexSkybox3D>(
        "skybox", vkr::scene::skyboxCubeVertices(),
        vkr::scene::skyboxCubeIndices());
    scene->createUniformBuffer<UniformBuffer3DObject>("skybox", {});

    for (const char *part : CornellBoxParts) {
      std::string path = "objects/cornellbox/";
      path += part;
      path += ".obj";

      std::string meshName = "cornellbox.";
      meshName += part;
      scene->loadMesh<vkr::scene::Vertex3D>(std::move(meshName),
                                            assetSystem->resolveApp(path));
    }

    scene->createUniformBuffer<UniformBuffer3DObject>("cornellbox", {});
  }

  void buildGraph() override {
    auto skyboxDesc = vkr::exec::RasterPassDesc::offscreen(
        swapchain->width(), swapchain->height(), VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_D32_SFLOAT, "skybox",
        vkr::scene::VertexSkybox3D::vertexInputDesc());
    skyboxDesc.uniform(0, "skybox", VK_SHADER_STAGE_VERTEX_BIT)
        .cubemap(1, "skybox", VK_SHADER_STAGE_FRAGMENT_BIT)
        .mesh("skybox")
        .clearColor(0.0f, 0.0f, 0.0f, 1.0f)
        .clearDepth();
    skyboxDesc.graphicsPipeline
        .vertexShader(vkr::resource::ShaderModuleDesc::vertexGlslFile(
            assetSystem->resolveApp("shaders/skybox/skybox.vert").string()))
        .fragmentShader(vkr::resource::ShaderModuleDesc::fragmentGlslFile(
            assetSystem->resolveApp("shaders/skybox/skybox.frag").string()))
        .readOnlyDepth()
        .noCull();

    auto &skyboxPass = graph->raster("skybox", std::move(skyboxDesc));

    auto cornellDesc = vkr::exec::RasterPassDesc::offscreen(
        swapchain->width(), swapchain->height(), VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_D32_SFLOAT, "cornellbox",
        vkr::scene::Vertex3D::vertexInputDesc());
    cornellDesc.uniform(0, "cornellbox", VK_SHADER_STAGE_VERTEX_BIT)
        .clearColor(0.0f, 0.0f, 0.0f, 0.0f)
        .clearDepth();
    cornellDesc.graphicsPipeline
        .vertexShader(vkr::resource::ShaderModuleDesc::vertexGlslFile(
            assetSystem->resolveApp("shaders/cornell/cornell.vert").string()))
        .fragmentShader(vkr::resource::ShaderModuleDesc::fragmentGlslFile(
            assetSystem->resolveApp("shaders/cornell/cornell.frag").string()))
        .noCull();

    for (const char *part : CornellBoxParts) {
      std::string meshName = "cornellbox.";
      meshName += part;
      cornellDesc.mesh(meshName);
    }

    auto &cornellPass = graph->raster("cornellbox", std::move(cornellDesc));

    auto compositeDesc = vkr::exec::FullscreenPassDesc::postProcess(
        swapchain->width(), swapchain->height(), VK_FORMAT_R8G8B8A8_UNORM,
        "skybox-cornell-composite");
    compositeDesc.graphicsPipeline
        .vertexShader(vkr::resource::ShaderModuleDesc::vertexGlslFile(
            assetSystem->resolveApp("shaders/composite/composite.vert")
                .string()))
        .fragmentShader(vkr::resource::ShaderModuleDesc::fragmentGlslFile(
            assetSystem->resolveApp("shaders/composite/composite.frag")
                .string()));

    auto &compositePass =
        graph->composite("composite",
                         std::vector<vkr::exec::RenderPassSource>{
                             vkr::exec::RenderPassSource{skyboxPass},
                             vkr::exec::RenderPassSource{cornellPass}},
                         std::move(compositeDesc));
    graph->present(compositePass);
  }

  void onDraw() override {
    const uint32_t frameIndex = executor->frameIndex();
    const auto &viewport = ctx.ui.viewport;
    ctx.camera.aspectRatio =
        ctx.ui.layoutMode == vkr::ui::LayoutMode::Standard &&
                viewport.height > 0.0f
            ? viewport.width / viewport.height
            : ctx.window.ratio();

    UniformBuffer3DObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = camera->getView();
    ubo.proj = camera->getProjection();

    scene->uniformBuffer<UniformBuffer3DObject>("skybox").update(frameIndex,
                                                                 ubo);
    scene->uniformBuffer<UniformBuffer3DObject>("cornellbox")
        .update(frameIndex, ubo);
  }

  void configure() override {
    ctx = vkr::exec::RenderAppDesc::windowed("cornellbox", "Cornell Box");
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

VKR_APP_RUN(SkyboxApp)
