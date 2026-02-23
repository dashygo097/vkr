#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class TestApplication : public vkr::VulkanApplication {
private:
  const std::vector<vkr::resource::VertexTextured3D> vertices1 = {
      {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}};

  const std::vector<vkr::resource::VertexTextured3D> vertices2 = {
      {{-0.25f, -0.25f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
      {{0.25f, -0.25f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
      {{0.25f, 0.25f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
      {{-0.25f, 0.25f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

  const std::vector<uint16_t> indices1 = {0, 1, 2, 2, 3, 0};
  const std::vector<uint16_t> indices2 = {0, 1, 2, 2, 3, 0};

  void createUniforms() override {
    resourceManager->createUniformBuffer<vkr::resource::UniformBuffer3DObject>(
        "default", {});
    resourceManager->createTexture("image1", "assets/textures/image.jpg");
  }

  std::vector<vkr::pipeline::DescriptorBinding>
  createDescriptorBindings() override {
    return {{0, vkr::pipeline::DescriptorType::UniformBuffer, 1,
             VK_SHADER_STAGE_VERTEX_BIT},
            {1, vkr::pipeline::DescriptorType::CombinedImageSampler, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT}};
  }

  void bindDescriptorSets() override {
    auto defaultUBO = resourceManager->getUniformBuffer("default");
    auto textureImage = resourceManager->getTextureImage("image1");
    auto textureImageView = resourceManager->getTextureImageView("image1");
    auto textureSampler = resourceManager->getTextureSampler("image1");
    if (defaultUBO && descriptorSets) {
      descriptorSets->bindUniformBuffer(
          0, defaultUBO->buffers(),
          sizeof(vkr::resource::UniformBuffer3DObject));
    }
    if (textureImage && descriptorSets) {
      descriptorSets->bindImageSampler(
          1, {textureImageView->imageView(), textureImageView->imageView()},
          textureSampler->sampler());
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

    ctx.width = 800;
    ctx.height = 600;
    ctx.title = "Test";

    ctx.cameraEnabled = true;
    ctx.cameraMovementSpeed = 5.0f;
    ctx.cameraMouseSensitivity = 0.5f;
    ctx.cameraFov = 45.0f;
    ctx.cameraAspectRatio = ctx.width / static_cast<float>(ctx.height);
    ctx.cameraNearPlane = 0.01f;
    ctx.cameraFarPlane = 1000.0f;

    ctx.pipelineMode = vkr::pipeline::PipelineMode::Textured3D;
  }

  void onSetup() override {
    resourceManager->createVertexBuffer<vkr::resource::VertexTextured3D>(
        "vb1", vertices1);
    resourceManager->createIndexBuffer("ib1", indices1);
    resourceManager->createVertexBuffer<vkr::resource::VertexTextured3D>(
        "vb2", vertices2);
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
