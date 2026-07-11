#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class CornellBoxApp : public vkr::VulkanApplication {
private:
  auto createDescriptorBindings()
      -> std::vector<vkr::pipeline::DescriptorBinding> override {
    return {{"default", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
             VK_SHADER_STAGE_VERTEX_BIT}};
  }

  void createResources() override {
    vkr::resource::Mesh<vkr::resource::Vertex3D> light(*device, *commandPool);
    light.load(assetSystem->resolve("objects/cornellbox/light.obj"));
    vkr::resource::Mesh<vkr::resource::Vertex3D> floor(*device, *commandPool);
    floor.load(assetSystem->resolve("objects/cornellbox/floor.obj"));
    vkr::resource::Mesh<vkr::resource::Vertex3D> left(*device, *commandPool);
    left.load(assetSystem->resolve("objects/cornellbox/left.obj"));
    vkr::resource::Mesh<vkr::resource::Vertex3D> right(*device, *commandPool);
    right.load(assetSystem->resolve("objects/cornellbox/right.obj"));
    vkr::resource::Mesh<vkr::resource::Vertex3D> tallbox(*device, *commandPool);
    tallbox.load(assetSystem->resolve("objects/cornellbox/tallbox.obj"));
    vkr::resource::Mesh<vkr::resource::Vertex3D> shortbox(*device,
                                                          *commandPool);
    shortbox.load(assetSystem->resolve("objects/cornellbox/shortbox.obj"));

    resourceManager->createMesh("light", light);
    resourceManager->createMesh("floor", floor);
    resourceManager->createMesh("left", left);
    resourceManager->createMesh("right", right);
    resourceManager->createMesh("tallbox", tallbox);
    resourceManager->createMesh("shortbox", shortbox);

    resourceManager->createUniformBuffer<vkr::resource::UniformBuffer3DObject>(
        "default", {});
  }

  void createPipelines() override {
    vkr::pipeline::GraphicsPipelineDesc default3d{};
    default3d.name = "default3d";
    default3d.renderPass = offscreenRenderPass->renderPass();
    default3d.layout.setLayouts = {descriptorSetLayout->layoutRef()};
    default3d.vertexInput = vkr::resource::Vertex3D::vertexInputDesc();

    default3d.shaders = {
        vkr::pipeline::GraphicsShaderStageDesc::vertex(
            vkr::resource::ShaderModuleDesc::vertexGlslFile(
                assetSystem->resolve("shaders/default3d/default3d.vert")
                    .string())),
        vkr::pipeline::GraphicsShaderStageDesc::fragment(
            vkr::resource::ShaderModuleDesc::fragmentGlslFile(
                assetSystem->resolve("shaders/default3d/default3d.frag")
                    .string())),
    };

    default3d.depthStencil.depthTestEnable = VK_TRUE;
    default3d.depthStencil.depthWriteEnable = VK_TRUE;
    default3d.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    default3d.rasterization.cullMode = VK_CULL_MODE_BACK_BIT;

    pipelineLibrary->create(default3d);
  }

  void onDrawFrame(uint32_t currentImage) override {
    vkr::resource::UniformBuffer3DObject ubo{};
    ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    ubo.view = camera->getView();
    ubo.proj = camera->getProjection();

    resourceManager->getUniformBuffer("default")->updateRaw(currentImage, &ubo,
                                                            sizeof(ubo));

    if (ui->viewportInfo().height > 0 &&
        ui->layoutMode() == vkr::ui::LayoutMode::Standard) {
      ctx.camera.aspectRatio = ui->viewportInfo().width /
                               static_cast<float>(ui->viewportInfo().height);
    } else {
      ctx.camera.aspectRatio = ctx.window.ratio();
    }
  }

  void onConfigure() override {
    ctx.window = {
        .title = "Cornell Box",
        .width = 1200,
        .height = 900,
    };

    ctx.instance = {
        .name = "cornellbox",
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
  CornellBoxApp app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
