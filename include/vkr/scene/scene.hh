#pragma once

#include "vkr/core/command/buffers.hh"
#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/scene/frame_uniform_buffer_set.hh"
#include "vkr/scene/geometry/mesh.hh"
#include "vkr/scene/material/cubemap.hh"
#include "vkr/scene/material/texture.hh"
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
  Scene(const core::Device &device, const core::CommandPool &commandPool,
        const core::CommandBuffers &commandBuffers);
  ~Scene();

  Scene(const Scene &) = delete;
  auto operator=(const Scene &) -> Scene & = delete;

  // Uniform buffer management
  template <typename UniformType>
  auto createUniformBuffer(std::string name, const UniformType &initial)
      -> FrameUniformBufferSet<UniformType> & {
    validateNewResource(uniform_buffers_, name, "uniform buffer");

    auto buffer = std::make_unique<FrameUniformBufferSet<UniformType>>(
        device_, command_buffers_.size());
    for (uint32_t frameIndex = 0; frameIndex < command_buffers_.size();
         ++frameIndex) {
      buffer->update(frameIndex, initial);
    }

    auto &result = *buffer;
    uniform_buffers_.emplace(std::move(name), std::move(buffer));
    return result;
  }

  template <typename UniformType>
  [[nodiscard]] auto uniformBuffer(std::string_view name)
      -> FrameUniformBufferSet<UniformType> & {
    try {
      return dynamic_cast<FrameUniformBufferSet<UniformType> &>(
          uniformBuffer(name));
    } catch (const std::bad_cast &) {
      VKR_RES_ERROR("Uniform buffer '{}' has a different type", name);
    }
  }

  template <typename UniformType>
  [[nodiscard]] auto uniformBuffer(std::string_view name) const
      -> const FrameUniformBufferSet<UniformType> & {
    try {
      return dynamic_cast<const FrameUniformBufferSet<UniformType> &>(
          uniformBuffer(name));
    } catch (const std::bad_cast &) {
      VKR_RES_ERROR("Uniform buffer '{}' has a different type", name);
    }
  }

  [[nodiscard]] auto uniformBuffer(std::string_view name)
      -> IFrameUniformBufferSet &;

  [[nodiscard]] auto uniformBuffer(std::string_view name) const
      -> const IFrameUniformBufferSet &;

  [[nodiscard]] auto findUniformBuffer(std::string_view name)
      -> std::optional<std::reference_wrapper<IFrameUniformBufferSet>>;

  [[nodiscard]] auto findUniformBuffer(std::string_view name) const
      -> std::optional<std::reference_wrapper<const IFrameUniformBufferSet>>;

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
  auto createTexture(std::string name, TextureDesc desc) -> Texture &;

  auto loadTexture(std::string name, const std::filesystem::path &path)
      -> Texture &;

  [[nodiscard]] auto texture(std::string_view name) -> Texture &;

  [[nodiscard]] auto texture(std::string_view name) const -> const Texture &;

  [[nodiscard]] auto findTexture(std::string_view name)
      -> std::optional<std::reference_wrapper<Texture>>;

  [[nodiscard]] auto findTexture(std::string_view name) const
      -> std::optional<std::reference_wrapper<const Texture>>;

  auto createCubemap(std::string name, CubemapDesc desc) -> Cubemap &;

  auto createCubemap(std::string name,
                     const std::array<std::string, 6> &facePaths,
                     VkFormat format = VK_FORMAT_R8G8B8A8_SRGB) -> Cubemap &;

  [[nodiscard]] auto cubemap(std::string_view name) -> Cubemap &;

  [[nodiscard]] auto cubemap(std::string_view name) const -> const Cubemap &;

  [[nodiscard]] auto findCubemap(std::string_view name)
      -> std::optional<std::reference_wrapper<Cubemap>>;

  [[nodiscard]] auto findCubemap(std::string_view name) const
      -> std::optional<std::reference_wrapper<const Cubemap>>;

  void destroyTexture(std::string_view name);

  void destroyCubemap(std::string_view name);

  // Counts
  [[nodiscard]] auto uniformBufferCount() const noexcept -> size_t;

  [[nodiscard]] auto textureCount() const noexcept -> size_t;

  [[nodiscard]] auto cubemapCount() const noexcept -> size_t;

  // Names
  [[nodiscard]] auto listUniformBufferNames() const
      -> std::vector<std::string>;

  [[nodiscard]] auto listTextureNames() const -> std::vector<std::string>;

  [[nodiscard]] auto listCubemapNames() const -> std::vector<std::string>;

  [[nodiscard]] auto listMeshNames() const -> std::vector<std::string>;

private:
  template <typename ResourceType>
  using ResourceMap =
      std::unordered_map<std::string, std::unique_ptr<ResourceType>>;

  template <typename ResourceType>
  static void validateNewResource(const ResourceMap<ResourceType> &resources,
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
  const core::CommandBuffers &command_buffers_;

  // components
  ResourceMap<IFrameUniformBufferSet> uniform_buffers_{};
  ResourceMap<Texture> textures_{};
  ResourceMap<Cubemap> cubemaps_{};
  ResourceMap<IMesh> meshes_{};
  std::string selected_mesh_name_{};
};

} // namespace vkr::scene
