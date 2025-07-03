#pragma once

#include <array>
#include <glm/glm.hpp>

#include "ctx.hpp"

class Vertex {
public:
  Vertex(glm::vec2 pos, glm::vec3 color);
  ~Vertex();
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 2>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
  }
};

class VertexBuffer {
public:
  VertexBuffer(const std::vector<Vertex> &vertices, VkDevice device,
               VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
               VkQueue graphicsQueue);
  VertexBuffer(const std::vector<Vertex> &vertices, const VulkanContext &ctx);
  ~VertexBuffer();

  void create();
  void destroy();
  void update(const std::vector<Vertex> &newVertices);

  std::vector<Vertex> getVertices() const { return vertices; }
  VkBuffer getVkBuffer() const { return vertexBuffer; }
  VkDeviceMemory getVkBufferMemory() const { return memory; }

private:
  // dependencies
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkCommandPool commandPool;
  VkQueue graphicsQueue;

  // components
  std::vector<Vertex> vertices{};
  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
};
