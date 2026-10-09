#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
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

} // namespace

class TeapotApp : public RenderApplication {
private:
  void createResources() override {
    scene().loadMesh<VertexNormalTexture3D>(
        "teapot", resolve("objects/teapot/teapot.obj"));
    scene().loadTexture("teapot_texture",
                        resolve("objects/teapot/default.png"));
    scene().createUniformBuffer<UniformBuffer3DObject>("default", {});
  }

  void buildGraph() override {
    auto desc = RasterPassDesc::offscreen(
        "teapot-local", swapchain().extent2D(), VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_D32_SFLOAT, VertexNormalTexture3D::vertexInputDesc());
    desc.uniform(0, VK_SHADER_STAGE_VERTEX_BIT)
        .texture(1, VK_SHADER_STAGE_FRAGMENT_BIT)
        .clearColor(0.0f, 0.0f, 0.0f, 1.0f)
        .clearDepth();
    desc.pipeline
        .vertexShader(ShaderModuleDesc::vertexGlslFile(
            resolve("shaders/teapot/teapot.vert")))
        .fragmentShader(ShaderModuleDesc::fragmentGlslFile(
            resolve("shaders/teapot/teapot.frag")))
        .noCull();

    auto &rasterPass = graph().raster("raster", std::move(desc));
    rasterPass.uniform(0, scene().uniformBuffer("default"))
        .texture(1, scene().texture("teapot_texture"));

    auto postDesc = FullscreenPassDesc::postProcess(
        "postprocess", swapchain().extent2D(), VK_FORMAT_R8G8B8A8_UNORM);
    postDesc.pipeline
        .vertexShader(ShaderModuleDesc::vertexGlslFile(
            resolve("shaders/postprocess/postprocess.vert")))
        .fragmentShader(ShaderModuleDesc::fragmentGlslFile(
            resolve("shaders/postprocess/postprocess.frag")));

    auto &postProcessPass =
        graph().postProcess("postprocess", rasterPass, std::move(postDesc));
    graph().present(postProcessPass);
  }

  void onDraw() override {
    const uint32_t frameIndex = executor().frameIndex();
    const auto &viewport = ui().viewport();
    camera().aspect(ui().layoutMode() == vkr::ui::LayoutMode::Standard &&
                            viewport.height > 0.0f
                        ? viewport.width / viewport.height
                        : ctx.window.ratio());

    UniformBuffer3DObject ubo{};
    ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.4f, -7.0f));
    ubo.model = glm::scale(ubo.model, glm::vec3(0.04f));
    ubo.view = camera().getView();
    ubo.proj = camera().getProjection();

    scene().uniformBuffer<UniformBuffer3DObject>("default").update(frameIndex,
                                                                   ubo);
  }

  void configure() override {
    ctx = RenderAppDesc::windowed("teapot", "Teapot");
    ctx.camera = {
        .movementSpeed = 5.0f,
        .mouseSensitivity = 0.5f,
        .aspectRatio = ctx.window.ratio(),
    };
  }
};

VKR_APP_RUN(TeapotApp)
