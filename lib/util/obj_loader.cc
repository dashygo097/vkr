#include "vkr/util/obj_loader.hh"
#include "vkr/logger.hh"
#include <array>
#include <functional>
#include <limits>
#include <tiny_obj_loader.h>
#include <unordered_map>

namespace vkr::util {
namespace {

struct IndexHash {
  auto operator()(const std::array<int, 3> &indices) const noexcept -> size_t {
    size_t result = 0;
    for (const auto index : indices) {
      result ^= std::hash<int>{}(index) + size_t{0x9e3779b9} + (result << 6) +
                (result >> 2);
    }
    return result;
  }
};

} // namespace

auto loadObj(const std::filesystem::path &path) -> ObjMesh {
  tinyobj::ObjReaderConfig config{};
  config.mtl_search_path = path.parent_path().string();
  tinyobj::ObjReader reader;
  if (!reader.ParseFromFile(path.string(), config)) {
    VKR_UTIL_ERROR("Failed to load OBJ '{}': {}", path.string(),
                   reader.Error());
  }
  if (!reader.Warning().empty()) {
    VKR_UTIL_WARN("OBJ '{}': {}", path.string(), reader.Warning());
  }

  const auto &attrib = reader.GetAttrib();
  ObjMesh result;
  std::unordered_map<std::array<int, 3>, uint16_t, IndexHash> uniqueVertices;
  for (const auto &shape : reader.GetShapes()) {
    for (const auto &index : shape.mesh.indices) {
      const std::array<int, 3> key{index.vertex_index, index.normal_index,
                                   index.texcoord_index};
      const auto existing = uniqueVertices.find(key);
      if (existing != uniqueVertices.end()) {
        result.indices.push_back(existing->second);
        continue;
      }
      if (result.vertices.size() > std::numeric_limits<uint16_t>::max()) {
        VKR_UTIL_ERROR("OBJ '{}' exceeds the 16-bit index capacity",
                       path.string());
      }
      if (index.vertex_index < 0 || static_cast<size_t>(index.vertex_index) >=
                                        attrib.vertices.size() / 3) {
        VKR_UTIL_ERROR("OBJ '{}' contains an invalid position index",
                       path.string());
      }

      ObjVertex vertex;
      const auto position = static_cast<size_t>(index.vertex_index) * 3;
      vertex.pos = {attrib.vertices[position], attrib.vertices[position + 1],
                    attrib.vertices[position + 2]};
      if (position + 2 < attrib.colors.size()) {
        vertex.color = {attrib.colors[position], attrib.colors[position + 1],
                        attrib.colors[position + 2]};
      }
      if (index.normal_index >= 0) {
        const auto normal = static_cast<size_t>(index.normal_index);
        if (normal >= attrib.normals.size() / 3) {
          VKR_UTIL_ERROR("OBJ '{}' contains an invalid normal index",
                         path.string());
        }
        vertex.normal = {attrib.normals[normal * 3],
                         attrib.normals[normal * 3 + 1],
                         attrib.normals[normal * 3 + 2]};
      }
      if (index.texcoord_index >= 0) {
        const auto uv = static_cast<size_t>(index.texcoord_index);
        if (uv >= attrib.texcoords.size() / 2) {
          VKR_UTIL_ERROR("OBJ '{}' contains an invalid UV index",
                         path.string());
        }
        vertex.texCoord = {attrib.texcoords[uv * 2],
                           attrib.texcoords[uv * 2 + 1]};
      }
      const auto vertexIndex = static_cast<uint16_t>(result.vertices.size());
      uniqueVertices.emplace(key, vertexIndex);
      result.vertices.push_back(vertex);
      result.indices.push_back(vertexIndex);
    }
  }
  if (result.vertices.empty() || result.indices.empty()) {
    VKR_UTIL_ERROR("OBJ '{}' contains no triangles", path.string());
  }
  VKR_UTIL_INFO("Loaded OBJ '{}': {} vertices, {} indices", path.string(),
                result.vertices.size(), result.indices.size());
  return result;
}

} // namespace vkr::util
