#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vkr.hh>
#include <vulkan/vulkan.h>

namespace {

struct Rgb {
  uint8_t r{0};
  uint8_t g{0};
  uint8_t b{0};
};

struct UniformBuffer3DObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

auto mixByte(uint8_t a, uint8_t b, float t) -> uint8_t {
  return static_cast<uint8_t>(static_cast<float>(a) * (1.0f - t) +
                              static_cast<float>(b) * t);
}

void writePpmFace(const std::filesystem::path &path, Rgb base, Rgb accent) {
  constexpr uint32_t Size = 128;

  std::filesystem::create_directories(path.parent_path());

  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("failed to create cubemap face: " + path.string());
  }

  out << "P6\n" << Size << " " << Size << "\n255\n";

  for (uint32_t y = 0; y < Size; ++y) {
    for (uint32_t x = 0; x < Size; ++x) {
      const float u = static_cast<float>(x) / static_cast<float>(Size - 1);
      const float v = static_cast<float>(y) / static_cast<float>(Size - 1);
      const float t = 0.25f + 0.75f * (0.65f * u + 0.35f * v);

      const std::array<uint8_t, 3> pixel = {
          mixByte(base.r, accent.r, t),
          mixByte(base.g, accent.g, t),
          mixByte(base.b, accent.b, t),
      };
      out.write(reinterpret_cast<const char *>(pixel.data()), sizeof(pixel));
    }
  }
}

auto createDebugCubemapFaces() -> std::array<std::string, 6> {
  const auto dir =
      std::filesystem::temp_directory_path() / "vkr_skybox_debug_faces";

  const std::array<std::filesystem::path, 6> paths = {
      dir / "right.ppm",  dir / "left.ppm",  dir / "top.ppm",
      dir / "bottom.ppm", dir / "front.ppm", dir / "back.ppm",
  };

  writePpmFace(paths[0], Rgb{180, 30, 30}, Rgb{255, 180, 80});
  writePpmFace(paths[1], Rgb{30, 170, 180}, Rgb{180, 255, 255});
  writePpmFace(paths[2], Rgb{40, 180, 80}, Rgb{210, 255, 160});
  writePpmFace(paths[3], Rgb{180, 60, 180}, Rgb{255, 190, 255});
  writePpmFace(paths[4], Rgb{50, 90, 210}, Rgb{170, 210, 255});
  writePpmFace(paths[5], Rgb{210, 180, 40}, Rgb{255, 245, 150});

  std::array<std::string, 6> result{};
  for (size_t i = 0; i < paths.size(); ++i) {
    result[i] = paths[i].string();
  }

  return result;
}

} // namespace

class SkyboxApp : public vkr::exec::RenderApplication {
private:
  void createResources() override {
    scene->createCubemap("skybox", createDebugCubemapFaces(),
                         VK_FORMAT_R8G8B8A8_SRGB);

    vkr::scene::Mesh<vkr::scene::VertexSkybox3D> skybox(*device,
                                                        *graphicsCommandPool);
    skybox.load(vkr::scene::skyboxCubeVertices(),
                vkr::scene::skyboxCubeIndices());
    scene->createMesh("skybox", skybox);

    scene->createUniformBuffer<UniformBuffer3DObject>("skybox", {});
  }

  void buildGraph() override {
    vkr::exec::SkyboxPassDesc skyboxDesc{};
    skyboxDesc.target = {
        .color = {.width = swapchain->width(),
                  .height = swapchain->height(),
                  .format = VK_FORMAT_R8G8B8A8_UNORM,
                  .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                           VK_IMAGE_USAGE_SAMPLED_BIT,
                  .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  .createSampler = true},
        .depth =
            vkr::exec::DepthAttachmentDesc{.width = swapchain->width(),
                                           .height = swapchain->height(),
                                           .format = VK_FORMAT_D32_SFLOAT}};
    skyboxDesc.clearValues = {
        VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
        VkClearValue{.depthStencil = {1.0f, 0}},
    };

    skyboxDesc.pipeline.setName("skybox")
        .vertexShader(vkr::resource::ShaderModuleDesc::vertexGlslFile(
            assetSystem->resolveApp("shaders/skybox/skybox.vert").string()))
        .fragmentShader(vkr::resource::ShaderModuleDesc::fragmentGlslFile(
            assetSystem->resolveApp("shaders/skybox/skybox.frag").string()));

    auto &skyboxPass = graph->addPass<vkr::exec::SkyboxPass>(
        *executor, *device, *graphicsCommandPool, *scene);
    skyboxPass.setName("skybox").write("scene.color");
    skyboxPass.update(skyboxDesc);

    vkr::exec::UiPassDesc uiDesc{};
    uiDesc.layoutMode = ctx.ui.layoutMode;
    uiDesc.descriptorPool = {
        .poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
                      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16}},
        .maxSets = ctx.commandBuffers.size};
    uiDesc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}};

    auto &uiPass = graph->addPass<vkr::exec::UiPass>(
        *executor, *window, *instance, *surface, *device, *graphicsCommandPool,
        *swapchain, *scene, *assetSystem, ctx.camera,
        vkr::exec::FullscreenPassSource{skyboxPass}, *graph, *timer, ctx.ui);
    uiPass.setName("ui").read("scene.color").write("swapchain");
    uiPass.update(uiDesc);

    auto &presentPass = graph->addPass<vkr::exec::PresentPass>(*executor);
    presentPass.setName("present");
  }

  void onDraw() override {
    const uint32_t frameIndex = executor->frameIndex();

    UniformBuffer3DObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = camera->getView();
    ubo.proj = camera->getProjection();

    scene->getUniformBuffer("skybox")->updateRaw(frameIndex, &ubo, sizeof(ubo));

    if (ctx.ui.viewport.height > 0 &&
        ctx.ui.layoutMode == vkr::ui::LayoutMode::Standard) {
      ctx.camera.aspectRatio =
          ctx.ui.viewport.width / static_cast<float>(ctx.ui.viewport.height);
    } else {
      ctx.camera.aspectRatio = ctx.window.ratio();
    }
  }

  void configure() override {
    ctx.window = {
        .title = "Skybox",
        .width = 1200,
        .height = 900,
    };

    ctx.instance = {
        .name = "skybox",
        .version = VK_MAKE_VERSION(1, 0, 0),
        .surfaceIntegration = vkr::core::SurfaceIntegration::GLFW,
    };
    ctx.commandBuffers.size = 2;

    ctx.camera = {
        .movementSpeed = 5.0f,
        .mouseSensitivity = 0.5f,
        .aspectRatio = ctx.window.ratio(),
    };
  }
};

auto main() -> int {
  SkyboxApp app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
