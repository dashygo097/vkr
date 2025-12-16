#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class CornellBoxApp : public vkr::VulkanApplication {
private:
  std::vector<vkr::DescriptorBinding> createDescriptorBindings() override {
    return {
        {0, vkr::DescriptorType::UniformBuffer, 1, VK_SHADER_STAGE_VERTEX_BIT}};
  }

  void bindDescriptors() override {
    auto defaultUBO = resourceManager->getUniformBuffer("default");
    if (defaultUBO && descriptor) {
      descriptor->bindUniformBuffer(0, defaultUBO->buffers(),
                                    sizeof(vkr::UniformBuffer3DObject));
    }
  }

  void updateUniformBuffer(uint32_t currentImage) override {
    vkr::UniformBuffer3DObject ubo{};
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

    ctx.vertexShaderPath = "shaders/albedo/vert.spv";
    ctx.fragmentShaderPath = "shaders/albedo/frag.spv";
  }

  void setting() override {
    vkr::geometry::Mesh light("./assets/light.obj", ctx);
    vkr::geometry::Mesh floor("./assets/floor.obj", ctx);
    vkr::geometry::Mesh left("./assets/left.obj", ctx);
    vkr::geometry::Mesh right("./assets/left.obj", ctx);
    vkr::geometry::Mesh tallbox("./assets/tallbox.obj", ctx);
    vkr::geometry::Mesh shortbox("./assets/shortbox.obj", ctx);

    resourceManager->createVertexBuffer("light",
                                        light.vertexBuffer()->vertices());
    resourceManager->createIndexBuffer("light", light.indexBuffer()->indices());
    resourceManager->createVertexBuffer("floor",
                                        floor.vertexBuffer()->vertices());
    resourceManager->createIndexBuffer("floor", floor.indexBuffer()->indices());
    resourceManager->createVertexBuffer("left",
                                        left.vertexBuffer()->vertices());
    resourceManager->createIndexBuffer("left", left.indexBuffer()->indices());
    resourceManager->createVertexBuffer("right",
                                        right.vertexBuffer()->vertices());
    resourceManager->createIndexBuffer("right", right.indexBuffer()->indices());
    resourceManager->createVertexBuffer("shortbox",
                                        shortbox.vertexBuffer()->vertices());
    resourceManager->createIndexBuffer("shortbox",
                                       shortbox.indexBuffer()->indices());
    resourceManager->createVertexBuffer("tallbox",
                                        tallbox.vertexBuffer()->vertices());
    resourceManager->createIndexBuffer("tallbox",
                                       tallbox.indexBuffer()->indices());
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
