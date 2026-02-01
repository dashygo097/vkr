#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class CornellBoxApp : public vkr::VulkanApplication {
private:
  std::vector<vkr::pipeline::DescriptorBinding>
  createDescriptorBindings() override {
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

    ctx.appName = "Vulkan App";
    ctx.appVersion = VK_MAKE_VERSION(1, 0, 0);

    ctx.width = 640;
    ctx.height = 480;
    ctx.title = "Cornell Box";

    ctx.cameraEnabled = true;
    ctx.cameraMovementSpeed = 5.0f;
    ctx.cameraMouseSensitivity = 0.5f;
    ctx.cameraFov = 45.0f;
    ctx.cameraAspectRatio = ctx.width / static_cast<float>(ctx.height);
    ctx.cameraNearPlane = 0.01f;
    ctx.cameraFarPlane = 1000.0f;

    ctx.vertexShaderPath = "shaders/albedo/vert_albedo.spv";
    ctx.fragmentShaderPath = "shaders/albedo/frag_albedo.spv";

    ctx.pipelineMode = vkr::pipeline::PipelineMode::Default3D;
  }

  void onSetup() override {
    vkr::resource::Mesh<vkr::resource::Vertex3D> light(*device, *commandPool);
    light.load("./assets/cornellbox/light.obj");
    vkr::resource::Mesh<vkr::resource::Vertex3D> floor(*device, *commandPool);
    floor.load("./assets/cornellbox/floor.obj");
    vkr::resource::Mesh<vkr::resource::Vertex3D> left(*device, *commandPool);
    left.load("./assets/cornellbox/left.obj");
    vkr::resource::Mesh<vkr::resource::Vertex3D> right(*device, *commandPool);
    right.load("./assets/cornellbox/right.obj");
    vkr::resource::Mesh<vkr::resource::Vertex3D> tallbox(*device, *commandPool);
    tallbox.load("./assets/cornellbox/tallbox.obj");
    vkr::resource::Mesh<vkr::resource::Vertex3D> shortbox(*device,
                                                          *commandPool);
    shortbox.load("./assets/cornellbox/shortbox.obj");

    resourceManager->createMesh("light", light);
    resourceManager->createMesh("floor", floor);
    resourceManager->createMesh("left", left);
    resourceManager->createMesh("right", right);
    resourceManager->createMesh("tallbox", tallbox);
    resourceManager->createMesh("shortbox", shortbox);
  }
};

int main() {
  CornellBoxApp app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
