#include "vkr/resources/geometry/mesh.hh"
#include <iostream>
#include <tiny_obj_loader.h>

namespace vkr::geometry {
Mesh::Mesh(VkDevice device, VkPhysicalDevice physicalDevice,
           VkCommandPool commandPool, VkQueue graphicsQueue)
    : device(device), physicalDevice(physicalDevice), commandPool(commandPool),
      graphicsQueue(graphicsQueue) {}

Mesh::Mesh(const VulkanContext &ctx)
    : device(ctx.device), physicalDevice(ctx.physicalDevice),
      commandPool(ctx.commandPool), graphicsQueue(ctx.graphicsQueue) {}

void Mesh::load(const std::vector<Vertex> &vertices,
                const std::vector<uint16_t> &indices) {
  checkVulkanContext();
  if (!_vertexBuffer || !_indexBuffer) {
    _vertexBuffer = std::make_unique<VertexBuffer>(
        vertices, device, physicalDevice, commandPool, graphicsQueue);
    _indexBuffer = std::make_unique<IndexBuffer>(
        indices, device, physicalDevice, commandPool, graphicsQueue);
  } else {
    update(vertices, indices);
  }
}

void Mesh::load(const std::string &meshFilePath) {
  checkVulkanContext();
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  // Extract directory for MTL files
  std::string dir = meshFilePath.substr(0, meshFilePath.find_last_of("/\\"));
  const char *basepath = dir.empty() ? nullptr : dir.c_str();

  // Load OBJ file
  bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                                  meshFilePath.c_str(), basepath, true);

  if (!warn.empty()) {
    std::cout << "OBJ warning: " << warn << std::endl;
  }

  if (!err.empty()) {
    std::cerr << "OBJ error: " << err << std::endl;
  }

  if (!success) {
    throw std::runtime_error("Failed to load OBJ file: " + meshFilePath);
  }

  // Parse loaded data
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  std::unordered_map<Vertex, uint16_t> uniqueVertices;

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      glm::vec3 pos{};
      glm::vec3 color{};

      if (index.vertex_index >= 0) {
        pos = {attrib.vertices[3 * index.vertex_index + 0],
               attrib.vertices[3 * index.vertex_index + 1],
               attrib.vertices[3 * index.vertex_index + 2]};
      }

      if (index.vertex_index >= 0 &&
          attrib.colors.size() > 3 * index.vertex_index + 2) {
        color = {attrib.colors[3 * index.vertex_index + 0],
                 attrib.colors[3 * index.vertex_index + 1],
                 attrib.colors[3 * index.vertex_index + 2]};
      } else {
        color = {1.0f, 1.0f, 1.0f}; // Default white
      }
      Vertex vertex(pos, color);

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint16_t>(vertices.size());
        vertices.push_back(vertex);
      }

      indices.push_back(uniqueVertices[vertex]);
    }
  }

  if (!_vertexBuffer || !_indexBuffer) {
    _vertexBuffer = std::make_unique<VertexBuffer>(
        vertices, device, physicalDevice, commandPool, graphicsQueue);
    _indexBuffer = std::make_unique<IndexBuffer>(
        indices, device, physicalDevice, commandPool, graphicsQueue);
  } else {
    update(vertices, indices);
  }

  std::cout << "Loaded mesh: " << vertices.size() << " vertices, "
            << indices.size() << " indices" << std::endl;
}

} // namespace vkr::geometry
