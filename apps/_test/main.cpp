#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class TestApp : public vkr::VulkanApplication {
private:
  const std::vector<vkr::resource::VertexTextured3D> vertices1 = {
      vkr::resource::VertexTextured3D({-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}),
      vkr::resource::VertexTextured3D({0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}),
      vkr::resource::VertexTextured3D({0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}),
      vkr::resource::VertexTextured3D({-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f})};

  const std::vector<vkr::resource::VertexTextured3D> vertices2 = {
      vkr::resource::VertexTextured3D({-0.25f, -0.25f, 0.5f},
                                      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}),
      vkr::resource::VertexTextured3D({0.25f, -0.25f, 0.5f}, {1.0f, 1.0f, 1.0f},
                                      {1.0f, 0.0f}),
      vkr::resource::VertexTextured3D({0.25f, 0.25f, 0.5f}, {1.0f, 1.0f, 1.0f},
                                      {1.0f, 1.0f}),
      vkr::resource::VertexTextured3D({-0.25f, 0.25f, 0.5f}, {1.0f, 1.0f, 1.0f},
                                      {0.0f, 1.0f})};

  const std::vector<uint16_t> indices1 = {0, 1, 2, 2, 3, 0};
  const std::vector<uint16_t> indices2 = {0, 1, 2, 2, 3, 0};

  void createResources() override {
    resourceManager->createVertexBuffer<vkr::resource::VertexTextured3D>(
        "vb1", vertices1);
    resourceManager->createIndexBuffer("ib1", indices1);
    resourceManager->createVertexBuffer<vkr::resource::VertexTextured3D>(
        "vb2", vertices2);
    resourceManager->createIndexBuffer("ib2", indices2);

    resourceManager->createUniformBuffer<vkr::resource::UniformBuffer3DObject>(
        "default", {});

    resourceManager->createTexture(
        "image1", assetSystem->resolve("textures/avatar.jpg").string());
  }

  void buildRenderGraph() override {
    vkr::render::RasterPassDesc desc{};
    desc.graph = {.name = "raster", .writes = {"scene.color"}};
    desc.target = {
        .color = {.width = swapchain->width(),
                  .height = swapchain->height(),
                  .format = VK_FORMAT_R8G8B8A8_UNORM,
                  .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                           VK_IMAGE_USAGE_SAMPLED_BIT,
                  .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  .createSampler = true},
        .depth =
            vkr::resource::DepthAttachmentDesc{.width = swapchain->width(),
                                               .height = swapchain->height(),
                                               .format = VK_FORMAT_D32_SFLOAT}};
    desc.descriptorBindings = {
        {.name = "default",
         .layout = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                    VK_SHADER_STAGE_VERTEX_BIT}},
        {.name = "image1",
         .layout = {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                    VK_SHADER_STAGE_FRAGMENT_BIT}}};
    desc.descriptorPool = {
        .poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
                      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16}},
        .maxSets = vkr::core::MAX_FRAMES_IN_FLIGHT};
    desc.clearValues = {VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
                        VkClearValue{.depthStencil = {1.0f, 0}}};

    desc.pipeline = [this](const vkr::render::RasterPipelineBuildInfo &info) {
      vkr::pipeline::GraphicsPipelineDesc textured{};
      textured.name = "textured";
      textured.renderPass = info.renderPass;
      textured.layout.setLayouts = {info.descriptorSetLayout};
      textured.vertexInput = vkr::resource::VertexTextured3D::vertexInputDesc();

      textured.shaders = {
          vkr::pipeline::GraphicsShaderStageDesc::vertex(
              vkr::resource::ShaderModuleDesc::vertexGlslFile(
                  assetSystem->resolve("shaders/texture3d/texture3d.vert")
                      .string())),
          vkr::pipeline::GraphicsShaderStageDesc::fragment(
              vkr::resource::ShaderModuleDesc::fragmentGlslFile(
                  assetSystem->resolve("shaders/texture3d/texture3d.frag")
                      .string())),
      };

      textured.depthStencil.depthTestEnable = VK_TRUE;
      textured.depthStencil.depthWriteEnable = VK_TRUE;
      textured.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
      textured.rasterization.cullMode = VK_CULL_MODE_BACK_BIT;

      return textured;
    };

    auto &rasterPass = renderGraph->addPass<vkr::render::RasterPass>(
        *renderer, *device, *commandPool, *resourceManager);
    rasterPass.update(desc);

    addUiPass(rasterPass);

    auto &presentPass = renderGraph->addPass<vkr::render::PresentPass>();
    presentPass.update(vkr::render::RenderGraphPassDesc{
        .name = "present", .reads = {"swapchain"}});
  }

  void onDraw() override {
    const uint32_t frameIndex = renderer->frameIndex();

    vkr::resource::UniformBuffer3DObject ubo{};
    ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    ubo.view = camera->getView();
    ubo.proj = camera->getProjection();

    resourceManager->getUniformBuffer("default")->updateRaw(frameIndex, &ubo,
                                                            sizeof(ubo));

    if (uiPass()->viewportInfo().height > 0 &&
        uiPass()->layoutMode() == vkr::ui::LayoutMode::Standard) {
      ctx.camera.aspectRatio =
          uiPass()->viewportInfo().width /
          static_cast<float>(uiPass()->viewportInfo().height);
    } else {
      ctx.camera.aspectRatio = ctx.window.ratio();
    }
  }

  void configure() override {
    ctx.window = {
        .title = "Test",
        .width = 1200,
        .height = 900,
    };

    ctx.instance = {
        .name = "test",
        .version = VK_MAKE_VERSION(1, 0, 0),
    };

    ctx.camera = {
        .movementSpeed = 5.0f,
        .mouseSensitivity = 0.5f,
        .aspectRatio = ctx.window.ratio(),
    };
  }
};

auto main() -> int {
  TestApp app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
