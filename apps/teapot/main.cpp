#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class TeapotApp : public vkr::VulkanApplication {
private:
  void createResources() override {
    vkr::resource::Mesh<vkr::resource::VertexNormal3D> teapot(*device,
                                                              *commandPool);
    teapot.load(assetSystem->resolve("objects/teapot/teapot.obj"));

    resourceManager->createMesh("teapot", teapot);

    resourceManager->createUniformBuffer<vkr::resource::UniformBuffer3DObject>(
        "default", {});
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
        {.name = "default",
         .layout = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                    VK_SHADER_STAGE_VERTEX_BIT}},
    };
    desc.descriptorPool = {
        .poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
                      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16}},
        .maxSets = vkr::core::MAX_FRAMES_IN_FLIGHT};
    desc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
                        VkClearValue{.depthStencil = {1.0f, 0}}};

    vkr::pipeline::GraphicsPipelineDesc normal{};
    normal.name = "teapot-local";
    normal.vertexInput = vkr::resource::VertexNormal3D::vertexInputDesc();

    normal.shaders = {
        vkr::pipeline::GraphicsShaderStageDesc::vertex(
            vkr::resource::ShaderModuleDesc::vertexGlslFile(
                assetSystem->resolve("shaders/teapot/teapot.vert")
                    .string())),
        vkr::pipeline::GraphicsShaderStageDesc::fragment(
            vkr::resource::ShaderModuleDesc::fragmentGlslFile(
                assetSystem->resolve("shaders/teapot/teapot.frag")
                    .string())),
    };

    normal.depthStencil.depthTestEnable = VK_TRUE;
    normal.depthStencil.depthWriteEnable = VK_TRUE;
    normal.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    normal.rasterization.cullMode = VK_CULL_MODE_BACK_BIT;

    desc.pipeline = normal;

    auto &rasterPass = renderGraph->addPass<vkr::render::RasterPass>(
        *renderer, *device, *commandPool, *resourceManager);
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
    postDesc.clearValues = {
        VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}};

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
        *renderer, *device, *commandPool,
        vkr::render::FullscreenPassSource{rasterPass});
    postProcessPass.setName("postprocess")
        .read("scene.raw")
        .write("scene.color");
    postProcessPass.update(postDesc);

    addUiPass(vkr::render::FullscreenPassSource{postProcessPass});

    auto &presentPass = renderGraph->addPass<vkr::render::PresentPass>();
    presentPass.setName("present").read("swapchain");
  }

  void onDraw() override {
    const uint32_t frameIndex = renderer->frameIndex();

    vkr::resource::UniformBuffer3DObject ubo{};
    ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    ubo.view = camera->getView();
    ubo.proj = camera->getProjection();

    resourceManager->getUniformBuffer("default")->updateRaw(frameIndex, &ubo,
                                                            sizeof(ubo));

    if (uiPass()->viewportInfo().height > 0 &&
        uiPass()->layoutMode() == vkr::ui::LayoutMode::Standard) {
      ctx.camera.aspectRatio =
          uiPass()->viewportInfo().width /
          static_cast<float>(uiPass()->viewportInfo().height);
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
