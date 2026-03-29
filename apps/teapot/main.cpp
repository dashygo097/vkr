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
    return {{0, vkr::pipeline::DescriptorType::UniformBuffer, 1,
             VK_SHADER_STAGE_VERTEX_BIT}};
  }

  void createUniforms() override {
    resourceManager->createUniformBuffer<vkr::resource::UniformBuffer3DObject>(
        "default", {});
  }

  void bindDescriptorSets() override {
    auto defaultUBO = resourceManager->getUniformBuffer("default");
    if (defaultUBO && descriptorSets) {
      descriptorSets->bindUniformBuffer(
          0, defaultUBO->buffers(),
          sizeof(vkr::resource::UniformBuffer3DObject));
    }
  }

  void updateUniforms(uint32_t currentImage) override {
    vkr::resource::UniformBuffer3DObject ubo{};
    ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    ubo.view = camera->getView();
    ubo.proj = camera->getProjection();

    resourceManager->getUniformBuffer("default")->updateRaw(currentImage, &ubo,
                                                            sizeof(ubo));
  }

  void onConfigure() override {
    ctx.appName = "teapot";
    ctx.appVersion = VK_MAKE_VERSION(1, 0, 0);

    ctx.width = 1200;
    ctx.height = 900;
    ctx.title = "Teapot Demo";

    ctx.cameraEnabled = true;
    ctx.cameraMovementSpeed = 5.0f;
    ctx.cameraMouseSensitivity = 0.5f;
    ctx.cameraFov = 45.0f;
    ctx.cameraAspectRatio = ctx.width / static_cast<float>(ctx.height);
    ctx.cameraNearPlane = 0.01f;
    ctx.cameraFarPlane = 1000.0f;

    ctx.pipelineMode = vkr::pipeline::PipelineMode::Default3D;
  }

  void onSetup() override {
    vkr::resource::Mesh<vkr::resource::Vertex3D> teapot(*device, *commandPool);
    teapot.load("./assets/teapot/teapot.obj");

    resourceManager->createMesh("teapot", teapot);
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
