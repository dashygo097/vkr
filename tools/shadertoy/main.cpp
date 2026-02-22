#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

struct ShaderToyConfig {
  int width{640};
  int height{480};
};

class ShaderToyApplication : public vkr::VulkanApplication {
public:
  ShaderToyApplication(const ShaderToyConfig &config) : config(config) {}

private:
  std::chrono::high_resolution_clock::time_point startTime;
  std::chrono::high_resolution_clock::time_point lastFrameTime;
  float totalTime{0.0f};
  int frameCount{0};

  glm::vec4 mouseState{0.0f};
  bool mousePressed{false};

  ShaderToyConfig config;

  void createUniforms() override {
    resourceManager
        ->createUniformBuffer<vkr::resource::UniformBufferShaderToyObject>(
            "shadertoy", {});
  }

  std::vector<vkr::pipeline::DescriptorBinding>
  createDescriptorBindings() override {
    return {{0, vkr::pipeline::DescriptorType::UniformBuffer, 1,
             VK_SHADER_STAGE_FRAGMENT_BIT}};
  }

  void bindDescriptorSets() override {
    auto shadertoyUBO = resourceManager->getUniformBuffer("shadertoy");
    if (shadertoyUBO && descriptorSets) {
      descriptorSets->bindUniformBuffer(
          0, shadertoyUBO->buffers(),
          sizeof(vkr::resource::UniformBufferShaderToyObject));
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

    vkr::resource::UniformBufferShaderToyObject ubo{};
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

    ctx.width = config.width;
    ctx.height = config.height;
    ctx.title = "ShaderToy Viewer";

    ctx.cameraEnabled = false;

    startTime = std::chrono::high_resolution_clock::now();
    lastFrameTime = startTime;

    ctx.pipelineMode = vkr::pipeline::PipelineMode::NoVertices;
  }
};

void printUsage(const char *programName) {
  std::cout << "Usage: " << programName << " [options]\n"
            << "Options:\n"
            << "  -w, --width <width>      Window width (default: 640)\n"
            << "  -h, --height <height>    Window height (default: 480)\n"
            << "  --help                   Display this help message\n"
            << std::endl;
}

ShaderToyConfig parseArgs(int argc, char *argv[]) {
  ShaderToyConfig config;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--help") {
      printUsage(argv[0]);
      exit(0);
    } else if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
      config.width = std::atoi(argv[++i]);
      if (config.width <= 0) {
        std::cerr << "Error: Invalid width value\n";
        exit(EXIT_FAILURE);
      }
    } else if ((arg == "-h" || arg == "--height") && i + 1 < argc) {
      config.height = std::atoi(argv[++i]);
      if (config.height <= 0) {
        std::cerr << "Error: Invalid height value\n";
        exit(EXIT_FAILURE);
      }
    } else {
      std::cerr << "Error: Unknown argument '" << arg << "'\n";
      printUsage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  return config;
}

int main(int argc, char *argv[]) {
  ShaderToyConfig config = parseArgs(argc, argv);

  ShaderToyApplication app(config);
  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
