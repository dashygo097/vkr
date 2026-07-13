#pragma once

#include "vkr/core/command/command_pool.hh"
#include "vkr/core/device.hh"
#include "vkr/resource/buffers/index_buffer.hh"
#include "vkr/resource/buffers/uniform_buffer.hh"
#include "vkr/resource/buffers/vertex_buffer.hh"
#include "vkr/resource/gpu/texture.hh"
#include "vkr/resource/mesh.hh"

namespace vkr::resource {

class ResourceManager {
public:
  ResourceManager(const core::Device &device,
                  const core::CommandPool &commandPool)
      : device_(device), command_pool_(commandPool) {}
  ~ResourceManager() = default;

  ResourceManager(const ResourceManager &) = delete;
  auto operator=(const ResourceManager &) -> ResourceManager & = delete;

  // Vertex buffer management
  template <typename VBOType>
  void createVertexBuffer(const std::string &name,
                          const std::vector<VBOType> &vertices) {
    auto buffer =
        std::make_shared<VertexBuffer<VBOType>>(device_, command_pool_);
    buffer->update(vertices);
    vertex_buffers_[name] = std::move(buffer);
  }

  [[nodiscard]] auto getVertexBuffer(const std::string &name) const
      -> std::shared_ptr<IVertexBuffer> {
    auto it = vertex_buffers_.find(name);
    return it == vertex_buffers_.end() ? nullptr : it->second;
  }

  void destroyVertexBuffer(const std::string &name) {
    vertex_buffers_.erase(name);
  }

  // Index buffer management
  void createIndexBuffer(const std::string &name,
                         const std::vector<uint16_t> &indices) {
    auto buffer = std::make_shared<IndexBuffer>(device_, command_pool_);
    buffer->update(indices);
    index_buffers_[name] = std::move(buffer);
  }

  [[nodiscard]] auto getIndexBuffer(const std::string &name) const
      -> std::shared_ptr<IndexBuffer> {
    auto it = index_buffers_.find(name);
    return it == index_buffers_.end() ? nullptr : it->second;
  }

  void destroyIndexBuffer(const std::string &name) {
    index_buffers_.erase(name);
  }

  // Uniform buffer management
  template <typename UBOType>
  void createUniformBuffer(const std::string &name, const UBOType &ubo) {
    auto buffer = std::make_shared<UniformBuffer<UBOType>>(device_);
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
  void createMesh(const std::string &name, const Mesh<VBOType> &mesh) {
    createVertexBuffer<VBOType>(name, mesh.vertexBuffer()->vertices());
    createIndexBuffer(name, mesh.indexBuffer()->indices());
  }

  void destroyMesh(const std::string &name) {
    if (selected_mesh_name_ == name) {
      selected_mesh_name_.clear();
    }

    destroyVertexBuffer(name);
    destroyIndexBuffer(name);
  }

  [[nodiscard]] auto hasMesh(const std::string &name) const -> bool {
    return getVertexBuffer(name) != nullptr && getIndexBuffer(name) != nullptr;
  }

  [[nodiscard]] auto meshCount() const -> size_t {
    return listMeshNames().size();
  }

  [[nodiscard]] auto selectedMeshName() const noexcept -> const std::string & {
    return selected_mesh_name_;
  }

  void selectMesh(std::string name) {
    selected_mesh_name_ = hasMesh(name) ? std::move(name) : std::string{};
  }

  void clearSelectedMesh() { selected_mesh_name_.clear(); }

  // Texture management
  void createTexture(const std::string &name, const TextureDesc &desc) {
    auto texture = std::make_shared<Texture>(device_, command_pool_);
    texture->update(desc);
    textures_[name] = std::move(texture);
  }

  void createTexture(const std::string &name, TextureDesc &&desc) {
    auto texture = std::make_shared<Texture>(device_, command_pool_);
    texture->update(desc);
    textures_[name] = std::move(texture);
  }

  void createTexture(const std::string &name, const std::string &filePath) {
    createTexture(name, TextureDesc::textureFile(filePath));
  }

  [[nodiscard]] auto getTexture(const std::string &name) const
      -> std::shared_ptr<Texture> {
    auto it = textures_.find(name);
    return it == textures_.end() ? nullptr : it->second;
  }

  void destroyTexture(const std::string &name) { textures_.erase(name); }

  // Counts
  [[nodiscard]] auto vertexBufferCount() const noexcept -> size_t {
    return vertex_buffers_.size();
  }

  [[nodiscard]] auto indexBufferCount() const noexcept -> size_t {
    return index_buffers_.size();
  }

  [[nodiscard]] auto uniformBufferCount() const noexcept -> size_t {
    return uniform_buffers_.size();
  }

  [[nodiscard]] auto textureCount() const noexcept -> size_t {
    return textures_.size();
  }

  [[nodiscard]] auto textureImageCount() const noexcept -> size_t {
    return textures_.size();
  }

  // Names
  [[nodiscard]] auto listVertexBufferNames() const -> std::vector<std::string> {
    return listResourceNames(vertex_buffers_);
  }

  [[nodiscard]] auto listIndexBufferNames() const -> std::vector<std::string> {
    return listResourceNames(index_buffers_);
  }

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

  [[nodiscard]] auto listMeshNames() const -> std::vector<std::string> {
    std::vector<std::string> names;

    for (const auto &[name, _] : vertex_buffers_) {
      if (index_buffers_.find(name) != index_buffers_.end()) {
        names.push_back(name);
      }
    }

    return names;
  }

  // Lists
  [[nodiscard]] auto listVertexBuffers() const
      -> std::vector<std::shared_ptr<IVertexBuffer>> {
    return listResources(vertex_buffers_);
  }

  [[nodiscard]] auto listIndexBuffers() const
      -> std::vector<std::shared_ptr<IndexBuffer>> {
    return listResources(index_buffers_);
  }

  [[nodiscard]] auto listUniformBuffers() const
      -> std::vector<std::shared_ptr<IUniformBuffer>> {
    return listResources(uniform_buffers_);
  }

  [[nodiscard]] auto listTextures() const
      -> std::vector<std::shared_ptr<Texture>> {
    return listResources(textures_);
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
      const std::unordered_map<std::string, std::shared_ptr<ResourceType>>
          &resourceMap) -> std::vector<std::string> {
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
  std::unordered_map<std::string, std::shared_ptr<IVertexBuffer>>
      vertex_buffers_{};
  std::unordered_map<std::string, std::shared_ptr<IndexBuffer>>
      index_buffers_{};
  std::unordered_map<std::string, std::shared_ptr<IUniformBuffer>>
      uniform_buffers_{};
  std::unordered_map<std::string, std::shared_ptr<Texture>> textures_{};
  std::string selected_mesh_name_{};
};

} // namespace vkr::resource
