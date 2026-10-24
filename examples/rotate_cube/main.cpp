#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <vkr.hh>
#include <vkr/pipeline/targets/swapchain.hh>

using namespace vkr::util;
using namespace vkr::core;
using namespace vkr::exec;
using namespace vkr::res;
using namespace vkr::pipe;
using namespace vkr::scene;

struct UniformObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

auto main() -> int {
  vkr::Logger::init();

  // window
  WindowDesc windowDesc{};
  windowDesc.title = "rotate_cube";
  windowDesc.width = 600;
  windowDesc.height = 480;
  windowDesc.resizable = false;

  Window window{windowDesc};

  // instance
  InstanceDesc instDesc{};
  instDesc.name = "rotate_cube";
  instDesc.version = VK_MAKE_VERSION(1, 0, 0);
  instDesc.surfaceIntegration = SurfaceIntegration::GLFW;

  Instance instance{instDesc};

  // surface
  Surface surface{instance, window};

  // device
  DeviceDesc deviceDesc{};

  Device device{instance, surface, deviceDesc};

  // swapchain
  SwapchainDesc swapchainDesc{};
  swapchainDesc.width = window.width();
  swapchainDesc.height = window.height();
  swapchainDesc.imageCount = MAX_FRAMES_IN_FLIGHT;

  Swapchain swapchain{window, surface, device, swapchainDesc};

  // command pool
  CommandPoolDesc commandPoolDesc{};
  commandPoolDesc.queueRole = CommandQueueRole::Graphics;

  CommandPool commandPool{device, commandPoolDesc};

  // command buffers
  CommandBuffers commands{device, commandPool};
  commands.update({.size = MAX_FRAMES_IN_FLIGHT});

  // cube geometry
  VertexBuffer<Vertex3D> vertices{device, commandPool};
  vertices.update({
      Vertex3D{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.2f, 0.2f}},
      Vertex3D{{0.5f, -0.5f, -0.5f}, {0.2f, 1.0f, 0.2f}},
      Vertex3D{{0.5f, 0.5f, -0.5f}, {0.2f, 0.2f, 1.0f}},
      Vertex3D{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.2f}},
      Vertex3D{{-0.5f, -0.5f, 0.5f}, {1.0f, 0.2f, 1.0f}},
      Vertex3D{{0.5f, -0.5f, 0.5f}, {0.2f, 1.0f, 1.0f}},
      Vertex3D{{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
      Vertex3D{{-0.5f, 0.5f, 0.5f}, {0.5f, 0.3f, 1.0f}},
  });

  IndexBuffer indices{device, commandPool};
  indices.update({
      0, 2, 1, 0, 3, 2, // back
      4, 5, 6, 4, 6, 7, // front
      0, 4, 7, 0, 7, 3, // left
      1, 2, 6, 1, 6, 5, // right
      0, 1, 5, 0, 5, 4, // bottom
      3, 7, 6, 3, 6, 2, // top
  });

  // swapchain attachments / render pass / framebuffers
  SwapchainTarget target{device, swapchain};
  target.update(SwapchainTargetDesc::colorDepth(
      swapchain.width(), swapchain.height(), VK_FORMAT_D32_SFLOAT));

  RenderPass renderPass{device};
  renderPass.update(
      RenderPassDesc::makeSwapchain(swapchain.format(), VK_FORMAT_D32_SFLOAT));

  Framebuffers framebuffers{device, renderPass};
  FramebuffersDesc framebufferDesc{};
  framebufferDesc.extent(swapchain.width(), swapchain.height())
      .attachmentViews(target.attachmentViews());
  framebuffers.update(framebufferDesc);

  FrameUniformBuffers<UniformObject> uniforms{device, commands.size()};

  // descriptors
  DescriptorSetLayoutDesc layoutDesc{};
  layoutDesc.bindings.push_back(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}});
  DescriptorSetLayout layout{device};
  layout.update(layoutDesc);

  DescriptorPool pool{device};
  pool.update(DescriptorPoolDesc::sets(layoutDesc, commands.size()));

  std::vector<DescriptorSet> descriptorSets{};
  descriptorSets.reserve(commands.size());
  for (uint32_t frame = 0; frame < commands.size(); ++frame) {
    descriptorSets.emplace_back(device, pool, layout);
    descriptorSets.back().uniform(0, uniforms, frame).create();
  }

  // graphics pipeline
  const auto shaders = executableDir() / "assets" / "shaders";
  auto pipelineDesc =
      GraphicsPipelineDesc::mesh("cube", Vertex3D::vertexInputDesc());
  pipelineDesc.descriptorSetLayout(0, layout.layout())
      .vertexShader(ShaderModuleDesc::vertexGlslFile(shaders / "cube.vert"))
      .fragmentShader(ShaderModuleDesc::fragmentGlslFile(shaders / "cube.frag"))
      .noCull();

  GraphicsPipeline pipeline{device, renderPass};
  pipeline.update(pipelineDesc);

  // render executor
  std::vector<VkClearValue> clearValues(2);
  clearValues[0].color = {{0.025f, 0.025f, 0.04f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  RenderExecutor executor{device, swapchain, commandPool, commands};

  // input tracer / timer / camera
  InputTracer inputTracer{window.glfwWindow()};
  inputTracer.installCallbacks();

  Timer timer{};

  CameraDesc cameraDesc{};
  cameraDesc.movementSpeed = 5.0f;
  cameraDesc.mouseSensitivity = 0.5f;
  cameraDesc.aspectRatio =
      static_cast<float>(swapchain.width()) / swapchain.height();

  Camera camera{timer, inputTracer};
  camera.update(cameraDesc);

  // main loop
  while (!window.shouldClose()) {
    timer.beginFrame();
    inputTracer.beginFrame();
    window.pollEvents();
    inputTracer.update();
    if (window.shouldClose() || inputTracer.wasKeyPressed(GLFW_KEY_ESCAPE)) {
      break;
    }
    timer.update();
    camera.track();

    if (!executor.beginFrame()) {
      break;
    }
    const auto frame = executor.frameIndex();

    UniformObject ubo{};
    ubo.model = glm::translate(glm::mat4{1.0f}, {0.0f, 0.0f, -4.0f});
    ubo.model = glm::rotate(ubo.model, timer.elapsedTime(),
                            glm::vec3{0.0f, 1.0f, 0.0f});
    ubo.view = camera.view();
    ubo.proj = camera.proj();
    uniforms.update(frame, ubo);

    executor.beginPass(renderPass, framebuffers, clearValues,
                       executor.imageIndex());
    executor.setViewportAndScissor(swapchain.extent2D());
    executor.bindPipeline(pipeline, descriptorSets[frame], 0);
    executor.drawIndexed(vertices, indices);
    executor.endPass();

    executor.submitFrame();
    executor.presentFrame();
    executor.endFrame();

    timer.endFrame();
    if (executor.swapchainOutOfDate()) {
      break;
    }
  }

  device.waitIdle();
  return 0;
}
