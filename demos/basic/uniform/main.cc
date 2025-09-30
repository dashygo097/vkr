#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class UniformBufferTestApplication : public vkr::VulkanApplication {
private:
  const std::vector<vkr::Vertex> vertices1 = {
      {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
      {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};

  const std::vector<vkr::Vertex> vertices2 = {
      {{-1.5f, -1.5f}, {0.0f, 0.0f, 0.0f}},
      {{-0.5f, -1.5f}, {0.0f, 0.0f, 1.0f}},
      {{-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
      {{-1.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}};

  const std::vector<uint16_t> indices1 = {0, 1, 2, 2, 3, 0};
  const std::vector<uint16_t> indices2 = {0, 1, 2, 2, 3, 0};

  void configure() {

    ctx.appName = "Vulkan App";
    ctx.appVersion = VK_MAKE_VERSION(1, 0, 0);
    ctx.engineName = "No Engine";
    ctx.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    ctx.width = 800;
    ctx.height = 600;
    std::string title = "Vulkan (Default Title)";

    ctx.cameraEnabled = true;
    ctx.cameraMovementSpeed = 5.0f;
    ctx.cameraMouseSensitivity = 0.5f;
    ctx.cameraFov = 45.0f;
    ctx.cameraAspectRatio = ctx.width / static_cast<float>(ctx.height);
    ctx.cameraNearPlane = 0.01f;
    ctx.cameraFarPlane = 1000.0f;

    ctx.vertexShaderPath = "shaders/uniform/vert.spv";
    ctx.fragmentShaderPath = "shaders/uniform/frag.spv";
  }

  void setting() {
    vertexBuffers->push_back(
        std::make_unique<vkr::VertexBuffer>(vertices1, ctx));
    vertexBuffers->push_back(
        std::make_unique<vkr::VertexBuffer>(vertices2, ctx));
    indexBuffers->push_back(std::make_unique<vkr::IndexBuffer>(indices1, ctx));
    indexBuffers->push_back(std::make_unique<vkr::IndexBuffer>(indices2, ctx));
  }
};

int main() {
  UniformBufferTestApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
