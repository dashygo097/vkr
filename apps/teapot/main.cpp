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

  auto createRasterPassDesc() -> vkr::render::RasterPassDesc override {
    auto desc = VulkanApplication::createRasterPassDesc();
    desc.descriptorBindings = {
        {.name = "default",
         .layout = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                    VK_SHADER_STAGE_VERTEX_BIT}},
    };

    desc.pipeline = [this](const vkr::render::RasterPipelineBuildInfo &info) {
      vkr::pipeline::GraphicsPipelineDesc normal{};
      normal.name = "normal3d";
      normal.renderPass = info.renderPass;
      normal.layout.setLayouts = {info.descriptorSetLayout};
      normal.vertexInput = vkr::resource::VertexNormal3D::vertexInputDesc();

      normal.shaders = {
          vkr::pipeline::GraphicsShaderStageDesc::vertex(
              vkr::resource::ShaderModuleDesc::vertexGlslFile(
                  assetSystem->resolve("shaders/normal3d/normal3d.vert")
                      .string())),
          vkr::pipeline::GraphicsShaderStageDesc::fragment(
              vkr::resource::ShaderModuleDesc::fragmentGlslFile(
                  assetSystem->resolve("shaders/normal3d/normal3d.frag")
                      .string())),
      };

      normal.depthStencil.depthTestEnable = VK_TRUE;
      normal.depthStencil.depthWriteEnable = VK_TRUE;
      normal.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
      normal.rasterization.cullMode = VK_CULL_MODE_BACK_BIT;

      return normal;
    };

    return desc;
  }

  void onDrawFrame(uint32_t currentImage) override {
    vkr::resource::UniformBuffer3DObject ubo{};
    ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    ubo.view = camera->getView();
    ubo.proj = camera->getProjection();

    resourceManager->getUniformBuffer("default")->updateRaw(currentImage, &ubo,
                                                            sizeof(ubo));

    if (uiPass->viewportInfo().height > 0 &&
        uiPass->layoutMode() == vkr::ui::LayoutMode::Standard) {
      ctx.camera.aspectRatio =
          uiPass->viewportInfo().width /
          static_cast<float>(uiPass->viewportInfo().height);
    } else {
      ctx.camera.aspectRatio = ctx.window.ratio();
    }
  }

  void onConfigure() override {
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
