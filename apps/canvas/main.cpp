#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <vkr.hh>
#include <vulkan/vulkan.h>

class CanvasApp : public vkr::VulkanApplication {
  const std::vector<vkr::resource::Vertex2D> vertices = {
      vkr::resource::Vertex2D({0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}),
      vkr::resource::Vertex2D({1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
      vkr::resource::Vertex2D({1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}),
      vkr::resource::Vertex2D({0.0f, 1.0f}, {1.0f, 1.0f, 1.0f})};

  const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

private:
  void createResources() override {
    resourceManager->createVertexBuffer<vkr::resource::Vertex2D>("vertices",
                                                                 vertices);
    resourceManager->createIndexBuffer("indices", indices);

    resourceManager->createUniformBuffer<vkr::resource::UniformBuffer3DObject>(
        "default", {});
  }

  auto createDescriptorBindings()
      -> std::vector<vkr::pipeline::DescriptorBinding> override {
    return {{"default", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
             VK_SHADER_STAGE_VERTEX_BIT}};
  }

  void createPipelines() override {
    vkr::pipeline::GraphicsPipelineDesc canvas{};
    canvas.name = "canvas";
    canvas.renderPass = offscreenRenderPass->renderPass();
    canvas.layout.setLayouts = {descriptorSetLayout->layoutRef()};
    canvas.vertexInput = vkr::resource::Vertex2D::vertexInputDesc();

    canvas.shaders = {
        vkr::pipeline::GraphicsShaderStageDesc::vertex(
            vkr::resource::ShaderModuleDesc::vertexGlslFile(
                assetSystem->resolve("shaders/default2d/default2d.vert")
                    .string())),
        vkr::pipeline::GraphicsShaderStageDesc::fragment(
            vkr::resource::ShaderModuleDesc::fragmentGlslFile(
                assetSystem->resolve("shaders/default2d/default2d.frag")
                    .string())),
    };

    canvas.depthStencil = vkr::pipeline::GraphicsDepthStencilDesc::disabled();
    canvas.rasterization = vkr::pipeline::GraphicsRasterizationDesc::noCull();

    pipelineLibrary->create(canvas);
  }

  void onDrawFrame(uint32_t currentImage) override {
    vkr::resource::UniformBuffer3DObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = glm::mat4(1.0f);
    ubo.proj = glm::ortho(0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f);
    resourceManager->getUniformBuffer("default")->updateRaw(currentImage, &ubo,
                                                            sizeof(ubo));
  }

  void onConfigure() override {
    ctx.window = {
        .title = "Canvas",
        .width = 1200,
        .height = 900,
    };

    ctx.instance = {
        .name = "canvas",
        .version = VK_MAKE_VERSION(1, 0, 0),
    };

    ctx.camera = {
        .locked = true,
    };
  }
};

auto main() -> int {
  CanvasApp app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
