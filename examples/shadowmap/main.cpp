#include <array>
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
  alignas(16) glm::mat4 lightView;
  alignas(16) glm::mat4 lightProj;
  alignas(16) glm::mat4 lightViewProj;
  alignas(16) glm::vec3 lightPos;
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

  void buildGraph() override {}

  void onDraw() override {
    const uint32_t frameIndex = executor->frameIndex();

    UniformBuffer3DObject ubo{};
    ubo.model = glm::mat4(1.0f);
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
