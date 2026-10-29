#pragma once

#include "vkr/core/command/buffer.hh"
#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffers/uniform_buffer.hh"
#include "vkr/resource/image/cubemap.hh"
#include "vkr/resource/image/texture.hh"
#include "vkr/scene/geometry/mesh.hh"
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vkr::scene {

class Scene {
public:
  explicit Scene(const core::Device &device,
                 const core::CommandPool &commandPool,
                 const std::vector<core::CommandBuffer> &commandBuffers);
  ~Scene();

  Scene(const Scene &) = delete;
  auto operator=(const Scene &) -> Scene & = delete;

  [[nodiscard]] auto world() noexcept -> Transform & { return world_; }
  [[nodiscard]] auto world() const noexcept -> const Transform & {
    return world_;
  }

  // Uniform buffer management
  template <typename UniformType>
  auto createUniformBuffer(std::string name, const UniformType &initial)
      -> std::vector<res::UniformBuffer> & {
    validateNewResource(uniform_buffers_, name, "uniform buffer");

    std::vector<res::UniformBuffer> buffers;
    buffers.reserve(command_buffers_.size());
    for (size_t frame = 0; frame < command_buffers_.size(); ++frame) {
      buffers.emplace_back(device_, sizeof(UniformType));
      buffers.back().update(initial);
    }

    return uniform_buffers_.emplace(std::move(name), std::move(buffers))
        .first->second;
  }

  [[nodiscard]] auto uniformBuffer(std::string_view name)
      -> std::vector<res::UniformBuffer> &;

  [[nodiscard]] auto uniformBuffer(std::string_view name) const
      -> const std::vector<res::UniformBuffer> &;

  [[nodiscard]] auto findUniformBuffer(std::string_view name)
      -> std::optional<std::reference_wrapper<std::vector<res::UniformBuffer>>>;

  [[nodiscard]] auto findUniformBuffer(std::string_view name) const
      -> std::optional<
          std::reference_wrapper<const std::vector<res::UniformBuffer>>>;

  void destroyUniformBuffer(std::string_view name);

  // Mesh management
  template <typename VertexType>
  auto createMesh(std::string name, const std::vector<VertexType> &vertices,
                  const std::vector<uint16_t> &indices) -> Mesh<VertexType> & {
    validateNewResource(meshes_, name, "mesh");

    auto mesh = std::make_unique<Mesh<VertexType>>(device_, command_pool_);
    mesh->load(vertices, indices);

    auto &result = *mesh;
    meshes_.emplace(std::move(name), std::move(mesh));
    return result;
  }

  template <typename VertexType>
  auto loadMesh(std::string name, const std::filesystem::path &path)
      -> Mesh<VertexType> & {
    validateNewResource(meshes_, name, "mesh");

    auto mesh = std::make_unique<Mesh<VertexType>>(device_, command_pool_);
    mesh->load(path.string());

    auto &result = *mesh;
    meshes_.emplace(std::move(name), std::move(mesh));
    return result;
  }

  template <typename VertexType>
  [[nodiscard]] auto mesh(std::string_view name) -> Mesh<VertexType> & {
    try {
      return dynamic_cast<Mesh<VertexType> &>(mesh(name));
    } catch (const std::bad_cast &) {
      VKR_RES_ERROR("Mesh '{}' has a different vertex type", name);
    }
  }

  template <typename VertexType>
  [[nodiscard]] auto mesh(std::string_view name) const
      -> const Mesh<VertexType> & {
    try {
      return dynamic_cast<const Mesh<VertexType> &>(mesh(name));
    } catch (const std::bad_cast &) {
      VKR_RES_ERROR("Mesh '{}' has a different vertex type", name);
    }
  }

  [[nodiscard]] auto mesh(std::string_view name) -> IMesh &;

  [[nodiscard]] auto mesh(std::string_view name) const -> const IMesh &;

  [[nodiscard]] auto findMesh(std::string_view name)
      -> std::optional<std::reference_wrapper<IMesh>>;

  [[nodiscard]] auto findMesh(std::string_view name) const
      -> std::optional<std::reference_wrapper<const IMesh>>;

  void destroyMesh(std::string_view name);

  [[nodiscard]] auto hasMesh(std::string_view name) const -> bool;

  [[nodiscard]] auto meshCount() const noexcept -> size_t;

  [[nodiscard]] auto selectedMeshName() const noexcept -> const std::string &;

  void selectMesh(std::string name);

  void clearSelectedMesh();

  // Texture management
  auto createTexture(std::string name, res::TextureDesc desc) -> res::Texture &;

  auto loadTexture(std::string name, const std::filesystem::path &path)
      -> res::Texture &;

  [[nodiscard]] auto texture(std::string_view name) -> res::Texture &;

  [[nodiscard]] auto texture(std::string_view name) const
      -> const res::Texture &;

  [[nodiscard]] auto findTexture(std::string_view name)
      -> std::optional<std::reference_wrapper<res::Texture>>;

  [[nodiscard]] auto findTexture(std::string_view name) const
      -> std::optional<std::reference_wrapper<const res::Texture>>;

  auto createCubemap(std::string name, res::CubemapDesc desc) -> res::Cubemap &;

  auto createCubemap(std::string name,
                     const std::array<std::string, 6> &facePaths,
                     VkFormat format = VK_FORMAT_R8G8B8A8_SRGB)
      -> res::Cubemap &;

  [[nodiscard]] auto cubemap(std::string_view name) -> res::Cubemap &;

  [[nodiscard]] auto cubemap(std::string_view name) const
      -> const res::Cubemap &;

  [[nodiscard]] auto findCubemap(std::string_view name)
      -> std::optional<std::reference_wrapper<res::Cubemap>>;

  [[nodiscard]] auto findCubemap(std::string_view name) const
      -> std::optional<std::reference_wrapper<const res::Cubemap>>;

  void destroyTexture(std::string_view name);

  void destroyCubemap(std::string_view name);

  // Counts
  [[nodiscard]] auto uniformBufferCount() const noexcept -> size_t;

  [[nodiscard]] auto textureCount() const noexcept -> size_t;

  [[nodiscard]] auto cubemapCount() const noexcept -> size_t;

  // Names
  [[nodiscard]] auto listUniformBufferNames() const -> std::vector<std::string>;

  [[nodiscard]] auto listTextureNames() const -> std::vector<std::string>;

  [[nodiscard]] auto listCubemapNames() const -> std::vector<std::string>;

  [[nodiscard]] auto listMeshNames() const -> std::vector<std::string>;

private:
  template <typename ResourceType>
  using ResourceMap =
      std::unordered_map<std::string, std::unique_ptr<ResourceType>>;

  template <typename Resources>
  static void validateNewResource(const Resources &resources,
                                  std::string_view name,
                                  std::string_view kind) {
    if (name.empty()) {
      VKR_RES_ERROR("Cannot create {} with an empty name", kind);
    }

    if (resources.find(std::string(name)) != resources.end()) {
      VKR_RES_ERROR("{} resource already exists: {}", kind, name);
    }
  }

  template <typename ResourceType>
  [[nodiscard]] static auto findResource(ResourceMap<ResourceType> &resources,
                                         std::string_view name)
      -> std::optional<std::reference_wrapper<ResourceType>> {
    const auto it = resources.find(std::string(name));
    if (it == resources.end()) {
      return std::nullopt;
    }
    return *it->second;
  }

  template <typename ResourceType>
  [[nodiscard]] static auto
  findResource(const ResourceMap<ResourceType> &resources,
               std::string_view name)
      -> std::optional<std::reference_wrapper<const ResourceType>> {
    const auto it = resources.find(std::string(name));
    if (it == resources.end()) {
      return std::nullopt;
    }
    return *it->second;
  }

  template <typename ResourceType>
  [[nodiscard]] static auto listResourceNames(
      const std::unordered_map<std::string, ResourceType> &resourceMap)
      -> std::vector<std::string> {
    std::vector<std::string> names;
    names.reserve(resourceMap.size());

    for (const auto &[name, _] : resourceMap) {
      names.push_back(name);
    }

    return names;
  }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  const std::vector<core::CommandBuffer> &command_buffers_;

  // components
  Transform world_{};
  std::unordered_map<std::string, std::vector<res::UniformBuffer>>
      uniform_buffers_{};
  ResourceMap<res::Texture> textures_{};
  ResourceMap<res::Cubemap> cubemaps_{};
  ResourceMap<IMesh> meshes_{};
  std::string selected_mesh_name_{};
};

} // namespace vkr::scene
