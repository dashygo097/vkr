#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class TestApplication : public vkr::VulkanApplication {
private:
  const std::vector<vkr::Vertex> vertices1 = {
      {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}};

  const std::vector<vkr::Vertex> vertices2 = {
      {{-1.5f, -1.5f, 0.0f}, {0.0f, 0.0f, 0.0f}},
      {{-0.5f, -1.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
      {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
      {{-1.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}};

  const std::vector<uint16_t> indices1 = {0, 1, 2, 2, 3, 0};
  const std::vector<uint16_t> indices2 = {0, 1, 2, 2, 3, 0};

  std::vector<vkr::DescriptorBinding> createDescriptorBindings() override {
    return {
        {0, vkr::DescriptorType::UniformBuffer, 1, VK_SHADER_STAGE_VERTEX_BIT}};
  }

  void bindDescriptors() override {
    auto defaultUBO = resourceManager->getUniformBuffer("default");
    if (defaultUBO && descriptor) {
      descriptor->bindUniformBuffer(0, defaultUBO->buffers(),
                                    sizeof(vkr::UniformBufferObject3D));
    }
  }

  void updateUniformBuffer(uint32_t currentImage) override {
    vkr::UniformBufferObject3D ubo{};
    ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    ubo.view = camera->getView();
    ubo.proj = camera->getProjection();

    resourceManager->getUniformBuffer("default")->updateRaw(currentImage, &ubo,
                                                            sizeof(ubo));
  }

  void configure() override {

    ctx.appName = "Vulkan App";
    ctx.appVersion = VK_MAKE_VERSION(1, 0, 0);
    ctx.engineName = "No Engine";
    ctx.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    ctx.width = 1024;
    ctx.height = 768;
    std::string title = "Vulkan (Default Title)";

    ctx.cameraEnabled = true;
    ctx.cameraMovementSpeed = 5.0f;
    ctx.cameraMouseSensitivity = 0.5f;
    ctx.cameraFov = 45.0f;
    ctx.cameraAspectRatio = ctx.width / static_cast<float>(ctx.height);
    ctx.cameraNearPlane = 0.01f;
    ctx.cameraFarPlane = 1000.0f;

    ctx.vertexShaderPath = "shaders/albedo/vert.spv";
    ctx.fragmentShaderPath = "shaders/albedo/frag.spv";
  }

  void setting() override {
    resourceManager->createVertexBuffer("vb1", vertices1);
    resourceManager->createVertexBuffer("vb2", vertices2);
    resourceManager->createIndexBuffer("ib1", indices1);
    resourceManager->createIndexBuffer("ib2", indices2);
  }
};

int main() {
  TestApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
