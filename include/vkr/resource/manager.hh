#pragma once

#include "vkr/core/command/pool.hh"
#include "vkr/core/device.hh"
#include "vkr/logger.hh"
#include "vkr/resource/buffer/frame_uniform_buffers.hh"
#include "vkr/scene/geometry/mesh.hh"
#include "vkr/scene/material/cubemap.hh"
#include "vkr/scene/material/texture.hh"
#include <array>

namespace vkr::resource {

class ResourceManager {
public:
  ResourceManager(const core::Device &device,
                  const core::CommandPool &commandPool)
      : device_(device), command_pool_(commandPool) {}
  ~ResourceManager() = default;

  ResourceManager(const ResourceManager &) = delete;
  auto operator=(const ResourceManager &) -> ResourceManager & = delete;

  // Uniform buffer management
  template <typename UBOType>
  void createUniformBuffer(const std::string &name, const UBOType &ubo) {
    auto buffer = std::make_shared<FrameUniformBuffers<UBOType>>(device_);
    buffer->update(0, ubo);
    uniform_buffers_[name] = std::move(buffer);
  }

  [[nodiscard]] auto getUniformBuffer(const std::string &name) const
      -> std::shared_ptr<IUniformBuffer> {
    auto it = uniform_buffers_.find(name);
    return it == uniform_buffers_.end() ? nullptr : it->second;
  }

  void destroyUniformBuffer(const std::string &name) {
    uniform_buffers_.erase(name);
  }

  // Mesh management
  template <typename VBOType>
  void createMesh(const std::string &name, const scene::Mesh<VBOType> &mesh) {
    const auto vertexBuffer = mesh.vertexBuffer();
    const auto indexBuffer = mesh.indexBuffer();

    if (!vertexBuffer || !indexBuffer) {
      VKR_RES_ERROR("Cannot create mesh resource '{}' from invalid mesh", name);
    }

    auto stored = std::make_shared<scene::Mesh<VBOType>>(device_,
                                                         command_pool_);
    stored->load(vertexBuffer->get().vertices(), indexBuffer->get().indices());
    meshes_[name] = std::move(stored);
  }

  void destroyMesh(const std::string &name) {
    if (selected_mesh_name_ == name) {
      selected_mesh_name_.clear();
    }

    meshes_.erase(name);
  }

  [[nodiscard]] auto hasMesh(const std::string &name) const -> bool {
    auto it = meshes_.find(name);
    return it != meshes_.end() && it->second && it->second->isValid();
  }

  [[nodiscard]] auto getMesh(const std::string &name) const
      -> std::shared_ptr<scene::IMesh> {
    auto it = meshes_.find(name);
    return it == meshes_.end() ? nullptr : it->second;
  }

  [[nodiscard]] auto meshCount() const noexcept -> size_t {
    return meshes_.size();
  }

  [[nodiscard]] auto selectedMeshName() const noexcept -> const std::string & {
    return selected_mesh_name_;
  }

  void selectMesh(std::string name) {
    selected_mesh_name_ = hasMesh(name) ? std::move(name) : std::string{};
  }

  void clearSelectedMesh() { selected_mesh_name_.clear(); }

  // Texture management
  void createTexture(const std::string &name, const scene::TextureDesc &desc) {
    auto texture = std::make_shared<scene::Texture>(device_, command_pool_);
    texture->update(desc);
    textures_[name] = std::move(texture);
  }

  void createTexture(const std::string &name, scene::TextureDesc &&desc) {
    auto texture = std::make_shared<scene::Texture>(device_, command_pool_);
    texture->update(desc);
    textures_[name] = std::move(texture);
  }

  void createTexture(const std::string &name, const std::string &filePath) {
    createTexture(name, scene::TextureDesc::textureFile(filePath));
  }

  void createCubemap(const std::string &name,
                     const std::array<std::string, 6> &facePaths,
                     VkFormat format = VK_FORMAT_R8G8B8A8_SRGB) {
    createCubemap(name, scene::CubemapDesc::files(facePaths, format));
  }

  void createCubemap(const std::string &name, const scene::CubemapDesc &desc) {
    auto cubemap = std::make_shared<scene::Cubemap>(device_, command_pool_);
    cubemap->update(desc);
    cubemaps_[name] = std::move(cubemap);
  }

  void createCubemap(const std::string &name, scene::CubemapDesc &&desc) {
    auto cubemap = std::make_shared<scene::Cubemap>(device_, command_pool_);
    cubemap->update(desc);
    cubemaps_[name] = std::move(cubemap);
  }

  [[nodiscard]] auto getCubemap(const std::string &name) const
      -> std::shared_ptr<scene::Cubemap> {
    auto it = cubemaps_.find(name);
    return it == cubemaps_.end() ? nullptr : it->second;
  }

  [[nodiscard]] auto getTexture(const std::string &name) const
      -> std::shared_ptr<scene::Texture> {
    auto it = textures_.find(name);
    return it == textures_.end() ? nullptr : it->second;
  }

  void destroyTexture(const std::string &name) { textures_.erase(name); }

  void destroyCubemap(const std::string &name) { cubemaps_.erase(name); }

  // Counts
  [[nodiscard]] auto uniformBufferCount() const noexcept -> size_t {
    return uniform_buffers_.size();
  }

  [[nodiscard]] auto textureCount() const noexcept -> size_t {
    return textures_.size();
  }

  [[nodiscard]] auto textureImageCount() const noexcept -> size_t {
    return textures_.size();
  }

  [[nodiscard]] auto cubemapCount() const noexcept -> size_t {
    return cubemaps_.size();
  }

  // Names
  [[nodiscard]] auto listUniformBufferNames() const
      -> std::vector<std::string> {
    return listResourceNames(uniform_buffers_);
  }

  [[nodiscard]] auto listTextureNames() const -> std::vector<std::string> {
    return listResourceNames(textures_);
  }

  [[nodiscard]] auto listTextureImageNames() const -> std::vector<std::string> {
    return listTextureNames();
  }

  [[nodiscard]] auto listCubemapNames() const -> std::vector<std::string> {
    return listResourceNames(cubemaps_);
  }

  [[nodiscard]] auto listMeshNames() const -> std::vector<std::string> {
    return listResourceNames(meshes_);
  }

  // Lists
  [[nodiscard]] auto listUniformBuffers() const
      -> std::vector<std::shared_ptr<IUniformBuffer>> {
    return listResources(uniform_buffers_);
  }

  [[nodiscard]] auto listTextures() const
      -> std::vector<std::shared_ptr<scene::Texture>> {
    return listResources(textures_);
  }

  [[nodiscard]] auto listCubemaps() const
      -> std::vector<std::shared_ptr<scene::Cubemap>> {
    return listResources(cubemaps_);
  }

  [[nodiscard]] auto listMeshes() const
      -> std::vector<std::shared_ptr<scene::IMesh>> {
    return listResources(meshes_);
  }

private:
  template <typename ResourceType>
  [[nodiscard]] static auto listResources(
      const std::unordered_map<std::string, std::shared_ptr<ResourceType>>
          &resourceMap) -> std::vector<std::shared_ptr<ResourceType>> {
    std::vector<std::shared_ptr<ResourceType>> resources;
    resources.reserve(resourceMap.size());

    for (const auto &[_, resource] : resourceMap) {
      resources.push_back(resource);
    }

    return resources;
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

  // components
  std::unordered_map<std::string, std::shared_ptr<IUniformBuffer>>
      uniform_buffers_{};
  std::unordered_map<std::string, std::shared_ptr<scene::Texture>> textures_{};
  std::unordered_map<std::string, std::shared_ptr<scene::Cubemap>> cubemaps_{};
  std::unordered_map<std::string, std::shared_ptr<scene::IMesh>> meshes_{};
  std::string selected_mesh_name_{};
};

} // namespace vkr::resource
