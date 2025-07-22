#include "vkr/geometry/mesh.hpp"
#include <stdexcept>

namespace vkr::geometry {

Mesh::Mesh(const VulkanContext &ctx)
    : device(ctx.device), physicalDevice(ctx.physicalDevice),
      commandPool(ctx.commandPool), graphicsQueue(ctx.graphicsQueue) {}

void Mesh::load(const std::vector<Vertex> &vertices,
                const std::vector<uint16_t> &indices) {
  checkVulkanContext();
  if (!vertexBuffer || !indexBuffer) {
    vertexBuffer = std::make_unique<VertexBuffer>(
        vertices, device, physicalDevice, commandPool, graphicsQueue);
    indexBuffer = std::make_unique<IndexBuffer>(indices, device, physicalDevice,
                                                commandPool, graphicsQueue);
  } else {
    update(vertices, indices);
  }
}

void Mesh::load(const std::string &meshFilePath) {
  checkVulkanContext();
  std::runtime_error("Mesh loading from file is not implemented yet.");
}

} // namespace vkr::geometry
