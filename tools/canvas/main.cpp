#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class CanvasApplication : public vkr::VulkanApplication {
  const std::vector<vkr::resource::Vertex3D> vertices = {
      {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}};

  const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

private:
  void createUniforms() override {
    resourceManager->createUniformBuffer<vkr::resource::UniformBuffer3DObject>(
        "default", {});
  }

  std::vector<vkr::pipeline::DescriptorBinding>
  createDescriptorBindings() override {
    return {{0, vkr::pipeline::DescriptorType::UniformBuffer, 1,
             VK_SHADER_STAGE_VERTEX_BIT}};
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
    ctx.title = "Fortest";

    ctx.cameraEnabled = true;
    ctx.cameraMovementSpeed = 5.0f;
    ctx.cameraMouseSensitivity = 0.5f;
    ctx.cameraFov = 45.0f;
    ctx.cameraAspectRatio = ctx.width / static_cast<float>(ctx.height);
    ctx.cameraNearPlane = 0.01f;
    ctx.cameraFarPlane = 1000.0f;

    ctx.vertexShaderPath = "shaders/canvas/vert_canvas.spv";
    ctx.fragmentShaderPath = "shaders/canvas/frag_canvas.spv";

    ctx.pipelineMode = vkr::pipeline::PipelineMode::Default3D;
  }

  void onSetup() override {
    resourceManager->createVertexBuffer<vkr::resource::Vertex3D>("vertices",
                                                                 vertices);
    resourceManager->createIndexBuffer("indices", indices);
  };
};

int main() {
  CanvasApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
