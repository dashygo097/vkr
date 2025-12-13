#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class CornellBoxApp : public vkr::VulkanApplication {
private:
  void configure() {

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

  void setting() {
    vkr::geometry::Mesh light("./assets/light.obj", ctx);
    vkr::geometry::Mesh floor("./assets/floor.obj", ctx);
    vkr::geometry::Mesh left("./assets/left.obj", ctx);
    vkr::geometry::Mesh right("./assets/left.obj", ctx);
    vkr::geometry::Mesh tallbox("./assets/tallbox.obj", ctx);
    vkr::geometry::Mesh shortbox("./assets/shortbox.obj", ctx);

    vertexBuffers->clear();
    indexBuffers->clear();

    vertexBuffers->push_back(light.vertexBuffer());
    indexBuffers->push_back(light.indexBuffer());

    vertexBuffers->push_back(floor.vertexBuffer());
    indexBuffers->push_back(floor.indexBuffer());

    vertexBuffers->push_back(left.vertexBuffer());
    indexBuffers->push_back(left.indexBuffer());

    vertexBuffers->push_back(right.vertexBuffer());
    indexBuffers->push_back(right.indexBuffer());

    vertexBuffers->push_back(shortbox.vertexBuffer());
    indexBuffers->push_back(shortbox.indexBuffer());

    vertexBuffers->push_back(tallbox.vertexBuffer());
    indexBuffers->push_back(tallbox.indexBuffer());
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
