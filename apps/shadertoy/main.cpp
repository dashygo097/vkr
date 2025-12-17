#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class ShaderToyApplication : public vkr::VulkanApplication {
private:
  std::chrono::high_resolution_clock::time_point startTime;
  std::chrono::high_resolution_clock::time_point lastFrameTime;
  float totalTime{0.0f};
  int frameCount{0};

  glm::vec4 mouseState{0.0f};
  bool mousePressed{false};

  void createUniforms() override {
    resourceManager->createUniformBuffer<vkr::UniformBufferShaderToyObject>(
        "shadertoy", {});
  }

  std::vector<vkr::DescriptorBinding> createDescriptorBindings() override {
    return {{0, vkr::DescriptorType::UniformBuffer, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT}};
  }

  void bindDescriptorSets() override {
    auto shadertoyUBO = resourceManager->getUniformBuffer("shadertoy");
    if (shadertoyUBO && descriptorSets) {
      descriptorSets->bindUniformBuffer(
          0, shadertoyUBO->buffers(),
          sizeof(vkr::UniformBufferShaderToyObject));
    }
  }

  void updateUniforms(uint32_t currentImage) override {
    auto shadertoyUBO = resourceManager->getUniformBuffer("shadertoy");
    if (!shadertoyUBO) {
      return;
    }

    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime =
        std::chrono::duration<float, std::chrono::seconds::period>(
            currentTime - lastFrameTime)
            .count();
    totalTime = std::chrono::duration<float, std::chrono::seconds::period>(
                    currentTime - startTime)
                    .count();
    lastFrameTime = currentTime;

    double mouseX, mouseY;
    glfwGetCursorPos(window->glfwWindow(), &mouseX, &mouseY);

    int leftButton =
        glfwGetMouseButton(window->glfwWindow(), GLFW_MOUSE_BUTTON_LEFT);
    if (leftButton == GLFW_PRESS && !mousePressed) {
      mouseState.z = static_cast<float>(mouseX);
      mouseState.w = static_cast<float>(ctx.height - mouseY);
      mousePressed = true;
    } else if (leftButton == GLFW_RELEASE) {
      mousePressed = false;
    }

    mouseState.x = static_cast<float>(mouseX);
    mouseState.y = static_cast<float>(ctx.height - mouseY);

    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);

    vkr::UniformBufferShaderToyObject ubo{};
    ubo.iResolution = glm::vec3(
        static_cast<float>(ctx.width), static_cast<float>(ctx.height),
        static_cast<float>(ctx.width) / static_cast<float>(ctx.height));
    ubo.iTime = totalTime;
    ubo.iTimeDelta = deltaTime;
    ubo.iFrameRate = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 60.0f;
    ubo.iFrame = frameCount++;
    ubo.iMouse = mouseState;
    ubo.iDate = glm::vec4(static_cast<float>(now->tm_year + 1900),
                          static_cast<float>(now->tm_mon),
                          static_cast<float>(now->tm_mday),
                          static_cast<float>(now->tm_hour * 3600 +
                                             now->tm_min * 60 + now->tm_sec));

    shadertoyUBO->updateRaw(currentImage, &ubo, sizeof(ubo));
  }

  void onConfigure() override {
    ctx.appName = "ShaderToy";
    ctx.appVersion = VK_MAKE_VERSION(1, 0, 0);
    ctx.engineName = "No Engine";
    ctx.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    ctx.width = 640;
    ctx.height = 480;
    ctx.title = "ShaderToy Viewer";

    ctx.cameraEnabled = false;

    ctx.vertexShaderPath = "shaders/shadertoy-examples/vert.spv";
    ctx.fragmentShaderPath = "shaders/shadertoy-examples/frag.spv";

    startTime = std::chrono::high_resolution_clock::now();
    lastFrameTime = startTime;

    ctx.pipelineMode = vkr::PipelineMode::NoVertex;
  }
};

int main() {
  ShaderToyApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
