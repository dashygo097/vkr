#include <array>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <utility>
#include <vkr.hh>
#include <vulkan/vulkan.h>

using namespace vkr::exec;
using namespace vkr::resource;
using namespace vkr::scene;

namespace {

struct UniformBuffer3DObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

constexpr std::array<const char *, 6> CornellBoxParts{
    "floor", "left", "light", "right", "shortbox", "tallbox"};

} // namespace

class SkyboxApp : public RenderApplication {
private:
  void createResources() override {
    scene().createCubemap("skybox", skyboxFaces(), VK_FORMAT_R8G8B8A8_SRGB);
    scene().createMesh<VertexSkybox3D>("skybox", skyboxCubeVertices(),
                                       skyboxCubeIndices());
    scene().createUniformBuffer<UniformBuffer3DObject>("skybox", {});

    for (const char *part : CornellBoxParts) {
      std::string path = "objects/cornellbox/";
      path += part;
      path += ".obj";

      std::string meshName = "cornellbox.";
      meshName += part;
      scene().loadMesh<Vertex3D>(std::move(meshName), resolve(path));
    }

    scene().createUniformBuffer<UniformBuffer3DObject>("cornellbox", {});
  }

  void buildGraph() override {
    auto skyboxDesc = RasterPassDesc::offscreen(
        "skybox", swapchain().extent2D(), VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_D32_SFLOAT, VertexSkybox3D::vertexInputDesc());
    skyboxDesc.uniform(0, 0, VK_SHADER_STAGE_VERTEX_BIT)
        .texture(1, 0, VK_SHADER_STAGE_FRAGMENT_BIT)
        .mesh("skybox")
        .clearColor(0.0f, 0.0f, 0.0f, 1.0f)
        .clearDepth();
    skyboxDesc.pipeline
        .vertexShader(ShaderModuleDesc::vertexGlslFile(
            resolve("shaders/skybox/skybox.vert")))
        .fragmentShader(ShaderModuleDesc::fragmentGlslFile(
            resolve("shaders/skybox/skybox.frag")))
        .readOnlyDepth()
        .noCull();

    auto &skyboxPass = graph().raster("skybox", std::move(skyboxDesc));
    skyboxPass.uniform(0, 0, scene().uniformBuffer("skybox"))
        .texture(1, 0, scene().cubemap("skybox"));

    auto cornellDesc = RasterPassDesc::offscreen(
        "cornellbox", swapchain().extent2D(), VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_D32_SFLOAT, Vertex3D::vertexInputDesc());
    cornellDesc.uniform(0, 0, VK_SHADER_STAGE_VERTEX_BIT)
        .clearColor(0.0f, 0.0f, 0.0f, 0.0f)
        .clearDepth();
    cornellDesc.pipeline
        .vertexShader(ShaderModuleDesc::vertexGlslFile(
            resolve("shaders/cornell/cornell.vert")))
        .fragmentShader(ShaderModuleDesc::fragmentGlslFile(
            resolve("shaders/cornell/cornell.frag")))
        .noCull();

    for (const char *part : CornellBoxParts) {
      std::string meshName = "cornellbox.";
      meshName += part;
      cornellDesc.mesh(meshName);
    }

    auto &cornellPass = graph().raster("cornellbox", std::move(cornellDesc));
    cornellPass.uniform(0, 0, scene().uniformBuffer("cornellbox"));

    auto compositeDesc = FullscreenPassDesc::postProcess(
        "skybox-cornell-composite", swapchain().extent2D(),
        VK_FORMAT_R8G8B8A8_UNORM);
    compositeDesc.input(0, 0).input(0, 1);
    compositeDesc.pipeline
        .vertexShader(ShaderModuleDesc::vertexGlslFile(
            resolve("shaders/composite/composite.vert")))
        .fragmentShader(ShaderModuleDesc::fragmentGlslFile(
            resolve("shaders/composite/composite.frag")));

    auto &compositePass = graph().composite(
        "composite", {skyboxPass, cornellPass}, std::move(compositeDesc));
    graph().present(compositePass);
  }

  void onDraw() override {
    const uint32_t frameIndex = executor().frameIndex();
    const auto &viewport = ui().viewport();
    camera().aspect(ui().layoutMode() == vkr::ui::LayoutMode::Standard &&
                            viewport.height > 0.0f
                        ? viewport.width / viewport.height
                        : ctx.window.ratio());

    UniformBuffer3DObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = camera().getView();
    ubo.proj = camera().getProjection();

    scene().uniformBuffer<UniformBuffer3DObject>("skybox").update(frameIndex,
                                                                  ubo);
    scene()
        .uniformBuffer<UniformBuffer3DObject>("cornellbox")
        .update(frameIndex, ubo);
  }

  void configure() override {
    ctx = RenderAppDesc::windowed("cornellbox", "Cornell Box");
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
        resolve("textures/skybox/right.ppm").string(),
        resolve("textures/skybox/left.ppm").string(),
        resolve("textures/skybox/top.ppm").string(),
        resolve("textures/skybox/bottom.ppm").string(),
        resolve("textures/skybox/front.ppm").string(),
        resolve("textures/skybox/back.ppm").string(),
    };
  }
};

VKR_APP_RUN(SkyboxApp)
