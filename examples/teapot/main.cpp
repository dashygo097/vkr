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

class TeapotApp : public vkr::VulkanApplication {
private:
  void createResources() override {
    vkr::scene::Mesh<vkr::resource::VertexNormalTexture3D> teapot(
        *device, *graphicsCommandPool);
    teapot.load(assetSystem->resolve("objects/teapot/teapot.obj"));

    scene->createMesh("teapot", teapot);
    scene->createTexture(
        "teapot_texture",
        assetSystem->resolve("objects/teapot/default.png").string());

    scene->createUniformBuffer<UniformBuffer3DObject>("default", {});
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
            vkr::render::DepthAttachmentDesc{.width = swapchain->width(),
                                               .height = swapchain->height(),
                                               .format = VK_FORMAT_D32_SFLOAT}};
    desc.descriptorBindings = {
        {.name = "default",
         .layout = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                    VK_SHADER_STAGE_VERTEX_BIT}},
        {.name = "teapot_texture",
         .layout = {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    VK_SHADER_STAGE_FRAGMENT_BIT}},
    };
    desc.descriptorPool = {
        .poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
                      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16}},
        .maxSets = vkr::core::MAX_FRAMES_IN_FLIGHT};
    desc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
                        VkClearValue{.depthStencil = {1.0f, 0}}};

    vkr::pipeline::GraphicsPipelineDesc normal{};
    normal.name = "teapot-local";
    normal.vertexInput =
        vkr::resource::VertexNormalTexture3D::vertexInputDesc();

    normal.shaders = {
        vkr::pipeline::GraphicsShaderStageDesc::vertex(
            vkr::resource::ShaderModuleDesc::vertexGlslFile(
                assetSystem->resolve("shaders/teapot/teapot.vert").string())),
        vkr::pipeline::GraphicsShaderStageDesc::fragment(
            vkr::resource::ShaderModuleDesc::fragmentGlslFile(
                assetSystem->resolve("shaders/teapot/teapot.frag").string())),
    };

    normal.depthStencil.depthTestEnable = VK_TRUE;
    normal.depthStencil.depthWriteEnable = VK_TRUE;
    normal.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    normal.rasterization = vkr::pipeline::GraphicsRasterizationDesc::noCull();

    desc.pipeline = normal;

    auto &rasterPass = renderGraph->addPass<vkr::render::RasterPass>(
        *renderer, *device, *graphicsCommandPool, *scene);
    rasterPass.setName("raster").write("scene.raw");
    rasterPass.update(desc);

    vkr::render::FullscreenPassDesc postDesc{};
    postDesc.target = {
        .color = {.width = swapchain->width(),
                  .height = swapchain->height(),
                  .format = VK_FORMAT_R8G8B8A8_UNORM,
                  .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                           VK_IMAGE_USAGE_SAMPLED_BIT,
                  .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  .createSampler = true}};
    postDesc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}};

    vkr::pipeline::GraphicsPipelineDesc postPipeline{};
    postPipeline.name = "postprocess";
    postPipeline.vertexInput = vkr::resource::VertexInputDesc::none();
    postPipeline.shaders = {
        vkr::pipeline::GraphicsShaderStageDesc::vertex(
            vkr::resource::ShaderModuleDesc::vertexGlslFile(
                assetSystem->resolve("shaders/postprocess/postprocess.vert")
                    .string())),
        vkr::pipeline::GraphicsShaderStageDesc::fragment(
            vkr::resource::ShaderModuleDesc::fragmentGlslFile(
                assetSystem->resolve("shaders/postprocess/postprocess.frag")
                    .string())),
    };
    postPipeline.depthStencil =
        vkr::pipeline::GraphicsDepthStencilDesc::disabled();
    postPipeline.rasterization =
        vkr::pipeline::GraphicsRasterizationDesc::noCull();
    postDesc.pipeline = postPipeline;

    auto &postProcessPass = renderGraph->addPass<vkr::render::PostProcessPass>(
        *renderer, *device, *graphicsCommandPool,
        vkr::render::FullscreenPassSource{rasterPass});
    postProcessPass.setName("postprocess")
        .read("scene.raw")
        .write("scene.color");
    postProcessPass.update(postDesc);

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
        vkr::render::FullscreenPassSource{postProcessPass}, *renderGraph,
        *timer, ctx.ui);
    uiPass.setName("ui").read("scene.color").write("swapchain");
    uiPass.update(uiDesc);

    auto &presentPass =
        renderGraph->addPass<vkr::render::PresentPass>(*renderer);
    presentPass.setName("present");
  }

  void onDraw() override {
    const uint32_t frameIndex = renderer->frameIndex();

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
