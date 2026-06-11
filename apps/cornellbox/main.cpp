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
    light.load(ctx.assetsDir + "objects/cornellbox/light.obj");
    vkr::resource::Mesh<vkr::resource::Vertex3D> floor(*device, *commandPool);
    floor.load(ctx.assetsDir + "objects/cornellbox/floor.obj");
    vkr::resource::Mesh<vkr::resource::Vertex3D> left(*device, *commandPool);
    left.load(ctx.assetsDir + "objects/cornellbox/left.obj");
    vkr::resource::Mesh<vkr::resource::Vertex3D> right(*device, *commandPool);
    right.load(ctx.assetsDir + "objects/cornellbox/right.obj");
    vkr::resource::Mesh<vkr::resource::Vertex3D> tallbox(*device, *commandPool);
    tallbox.load(ctx.assetsDir + "objects/cornellbox/tallbox.obj");
    vkr::resource::Mesh<vkr::resource::Vertex3D> shortbox(*device,
                                                          *commandPool);
    shortbox.load(ctx.assetsDir + "objects/cornellbox/shortbox.obj");

    resourceManager->createMesh("light", light);
    resourceManager->createMesh("floor", floor);
    resourceManager->createMesh("left", left);
    resourceManager->createMesh("right", right);
    resourceManager->createMesh("tallbox", tallbox);
    resourceManager->createMesh("shortbox", shortbox);

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
      ctx.cameraAspectRatio = ui->viewportInfo().width /
                              static_cast<float>(ui->viewportInfo().height);
    } else {
      ctx.cameraAspectRatio = ctx.width / static_cast<float>(ctx.height);
    }
  }

  void onConfigure() override {
    ctx.appName = "cornellbox";
    ctx.appVersion = VK_MAKE_VERSION(1, 0, 0);

    ctx.width = 1200;
    ctx.height = 900;
    ctx.title = "Cornell Box";

    ctx.cameraEnabled = true;
    ctx.cameraMovementSpeed = 5.0f;
    ctx.cameraMouseSensitivity = 0.5f;
    ctx.cameraFov = 45.0f;
    ctx.cameraAspectRatio = ctx.width / static_cast<float>(ctx.height);
    ctx.cameraNearPlane = 0.01f;
    ctx.cameraFarPlane = 1000.0f;

    ctx.pipelineMode = vkr::pipeline::PipelineMode::Default3D;
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
