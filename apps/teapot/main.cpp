#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class TeapotApp : public vkr::VulkanApplication {
private:
  auto createDescriptorBindings()
      -> std::vector<vkr::pipeline::DescriptorBinding> override {
    return {{"default", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
             VK_SHADER_STAGE_VERTEX_BIT}};
  }

  void createResources() override {
    vkr::resource::Mesh<vkr::resource::VertexNormal3D> teapot(*device,
                                                              *commandPool);
    teapot.load(assetSystem->resolve("objects/teapot/teapot.obj"));

    resourceManager->createMesh("teapot", teapot);

    resourceManager->createUniformBuffer<vkr::resource::UniformBuffer3DObject>(
        "default", {});
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

    ctx.pipelineMode = vkr::pipeline::PipelineMode::Normal3D;
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
