#include "vkr/geometry/mesh.hh"
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
  if (!vertexBuffer || !indexBuffer) {
    vertexBuffer = std::make_unique<VertexBuffer>(
        vertices, device, physicalDevice, commandPool, graphicsQueue);
    indexBuffer = std::make_unique<IndexBuffer>(indices, device, physicalDevice,
                                                commandPool, graphicsQueue);
  } else {
    update(vertices, indices);
  }
}

void Mesh::load(const std::string &meshFilePath) { checkVulkanContext(); }

} // namespace vkr::geometry
