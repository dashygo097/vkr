#include "vkr/resources/mesh.hh"
#include <tiny_obj_loader.h>

namespace vkr::resource {

template <> void Mesh<Vertex3D>::load(const std::string &meshFilePath) {
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
    VKR_RES_WARN("OBJ warning: {}", warn)
  }

  if (!err.empty()) {
    VKR_RES_ERROR("OBJ error: {}", err);
  }

  if (!success) {
    VKR_RES_ERROR("Failed to load OBJ file: {}", meshFilePath);
  }

  // Parse loaded data
  std::vector<Vertex3D> vertices;
  std::vector<uint16_t> indices;

  std::unordered_map<Vertex3D, uint16_t> uniqueVertices;

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
      Vertex3D vertex;
      vertex.pos = pos;
      vertex.color = color;

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint16_t>(vertices.size());
        vertices.push_back(vertex);
      }

      indices.push_back(uniqueVertices[vertex]);
    }
  }

  if (!vertex_buffer_ || !index_buffer_) {
    vertex_buffer_ =
        std::make_unique<VertexBufferBase<Vertex3D>>(device_, command_pool_);
    vertex_buffer_->update(vertices);
    index_buffer_ = std::make_unique<IndexBuffer>(device_, command_pool_);
    index_buffer_->update(indices);
  } else {
    update(vertices, indices);
  }

  VKR_RES_INFO("Loaded mesh: {} vertices, {} indices", vertices.size(),
               indices.size());
}

} // namespace vkr::resource
