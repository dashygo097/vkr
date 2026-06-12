#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class TestApp : public vkr::VulkanApplication {
private:
  const std::vector<vkr::resource::VertexTextured3D> vertices1 = {
      vkr::resource::VertexTextured3D({-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}),
      vkr::resource::VertexTextured3D({0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}),
      vkr::resource::VertexTextured3D({0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}),
      vkr::resource::VertexTextured3D({-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f})};

  const std::vector<vkr::resource::VertexTextured3D> vertices2 = {
      vkr::resource::VertexTextured3D({-0.25f, -0.25f, 0.5f},
                                      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}),
      vkr::resource::VertexTextured3D({0.25f, -0.25f, 0.5f}, {1.0f, 1.0f, 1.0f},
                                      {1.0f, 0.0f}),
      vkr::resource::VertexTextured3D({0.25f, 0.25f, 0.5f}, {1.0f, 1.0f, 1.0f},
                                      {1.0f, 1.0f}),
      vkr::resource::VertexTextured3D({-0.25f, 0.25f, 0.5f}, {1.0f, 1.0f, 1.0f},
                                      {0.0f, 1.0f})};

  const std::vector<uint16_t> indices1 = {0, 1, 2, 2, 3, 0};
  const std::vector<uint16_t> indices2 = {0, 1, 2, 2, 3, 0};

  void createResources() override {
    resourceManager->createVertexBuffer<vkr::resource::VertexTextured3D>(
        "vb1", vertices1);
    resourceManager->createIndexBuffer("ib1", indices1);
    resourceManager->createVertexBuffer<vkr::resource::VertexTextured3D>(
        "vb2", vertices2);
    resourceManager->createIndexBuffer("ib2", indices2);

    resourceManager->createUniformBuffer<vkr::resource::UniformBuffer3DObject>(
        "default", {});

    resourceManager->createTexture("image1",
                                   ctx.assetsDir + "textures/avatar.jpg");
  }

  auto createDescriptorBindings()
      -> std::vector<vkr::pipeline::DescriptorBinding> override {
    return {{"default", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
             VK_SHADER_STAGE_VERTEX_BIT},
            {"image1", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT}};
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
        .title = "Test",
        .width = 1200,
        .height = 900,
    };

    ctx.instance = {
        .name = "test",
        .version = VK_MAKE_VERSION(1, 0, 0),
    };

    ctx.camera = {
        .movementSpeed = 5.0f,
        .mouseSensitivity = 0.5f,
        .aspectRatio = ctx.window.ratio(),
    };

    ctx.pipelineMode = vkr::pipeline::PipelineMode::Textured3D;
  }
};

auto main() -> int {
  TestApp app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
