#include "vkr/scene/scene.hh"

namespace vkr::scene {

Scene::Scene(const core::Device &device, const core::CommandPool &commandPool,
             const core::CommandBuffers &commandBuffers)
    : device_(device), command_pool_(commandPool),
      command_buffers_(commandBuffers) {
  if (command_buffers_.empty()) {
    VKR_RES_ERROR("Scene requires initialized command buffers");
  }
}

Scene::~Scene() = default;

auto Scene::uniformBuffer(std::string_view name) -> IFrameUniformBufferSet & {
  const auto resource = findUniformBuffer(name);
  if (!resource) {
    VKR_RES_ERROR("Uniform buffer resource not found: {}", name);
  }
  return resource->get();
}

auto Scene::uniformBuffer(std::string_view name) const
    -> const IFrameUniformBufferSet & {
  const auto resource = findUniformBuffer(name);
  if (!resource) {
    VKR_RES_ERROR("Uniform buffer resource not found: {}", name);
  }
  return resource->get();
}

auto Scene::findUniformBuffer(std::string_view name)
    -> std::optional<std::reference_wrapper<IFrameUniformBufferSet>> {
  return findResource(uniform_buffers_, name);
}

auto Scene::findUniformBuffer(std::string_view name) const
    -> std::optional<std::reference_wrapper<const IFrameUniformBufferSet>> {
  return findResource(uniform_buffers_, name);
}

void Scene::destroyUniformBuffer(std::string_view name) {
  uniform_buffers_.erase(std::string(name));
}

auto Scene::mesh(std::string_view name) -> IMesh & {
  const auto resource = findMesh(name);
  if (!resource) {
    VKR_RES_ERROR("Mesh resource not found: {}", name);
  }
  return resource->get();
}

auto Scene::mesh(std::string_view name) const -> const IMesh & {
  const auto resource = findMesh(name);
  if (!resource) {
    VKR_RES_ERROR("Mesh resource not found: {}", name);
  }
  return resource->get();
}

auto Scene::findMesh(std::string_view name)
    -> std::optional<std::reference_wrapper<IMesh>> {
  return findResource(meshes_, name);
}

auto Scene::findMesh(std::string_view name) const
    -> std::optional<std::reference_wrapper<const IMesh>> {
  return findResource(meshes_, name);
}

void Scene::destroyMesh(std::string_view name) {
  if (selected_mesh_name_ == name) {
    selected_mesh_name_.clear();
  }
  meshes_.erase(std::string(name));
}

auto Scene::hasMesh(std::string_view name) const -> bool {
  const auto resource = findMesh(name);
  return resource && resource->get().isValid();
}

auto Scene::meshCount() const noexcept -> size_t { return meshes_.size(); }

auto Scene::selectedMeshName() const noexcept -> const std::string & {
  return selected_mesh_name_;
}

void Scene::selectMesh(std::string name) {
  selected_mesh_name_ = hasMesh(name) ? std::move(name) : std::string{};
}

void Scene::clearSelectedMesh() { selected_mesh_name_.clear(); }

auto Scene::createTexture(std::string name, TextureDesc desc) -> Texture & {
  validateNewResource(textures_, name, "texture");

  auto texture = std::make_unique<Texture>(device_, command_pool_);
  texture->update(desc);

  auto &result = *texture;
  textures_.emplace(std::move(name), std::move(texture));
  return result;
}

auto Scene::loadTexture(std::string name, const std::filesystem::path &path)
    -> Texture & {
  return createTexture(std::move(name),
                       TextureDesc::textureFile(path.string()));
}

auto Scene::texture(std::string_view name) -> Texture & {
  const auto resource = findTexture(name);
  if (!resource) {
    VKR_RES_ERROR("Texture resource not found: {}", name);
  }
  return resource->get();
}

auto Scene::texture(std::string_view name) const -> const Texture & {
  const auto resource = findTexture(name);
  if (!resource) {
    VKR_RES_ERROR("Texture resource not found: {}", name);
  }
  return resource->get();
}

auto Scene::findTexture(std::string_view name)
    -> std::optional<std::reference_wrapper<Texture>> {
  return findResource(textures_, name);
}

auto Scene::findTexture(std::string_view name) const
    -> std::optional<std::reference_wrapper<const Texture>> {
  return findResource(textures_, name);
}

auto Scene::createCubemap(std::string name, CubemapDesc desc) -> Cubemap & {
  validateNewResource(cubemaps_, name, "cubemap");

  auto cubemap = std::make_unique<Cubemap>(device_, command_pool_);
  cubemap->update(desc);

  auto &result = *cubemap;
  cubemaps_.emplace(std::move(name), std::move(cubemap));
  return result;
}

auto Scene::createCubemap(std::string name,
                          const std::array<std::string, 6> &facePaths,
                          VkFormat format) -> Cubemap & {
  return createCubemap(std::move(name), CubemapDesc::files(facePaths, format));
}

auto Scene::cubemap(std::string_view name) -> Cubemap & {
  const auto resource = findCubemap(name);
  if (!resource) {
    VKR_RES_ERROR("Cubemap resource not found: {}", name);
  }
  return resource->get();
}

auto Scene::cubemap(std::string_view name) const -> const Cubemap & {
  const auto resource = findCubemap(name);
  if (!resource) {
    VKR_RES_ERROR("Cubemap resource not found: {}", name);
  }
  return resource->get();
}

auto Scene::findCubemap(std::string_view name)
    -> std::optional<std::reference_wrapper<Cubemap>> {
  return findResource(cubemaps_, name);
}

auto Scene::findCubemap(std::string_view name) const
    -> std::optional<std::reference_wrapper<const Cubemap>> {
  return findResource(cubemaps_, name);
}

void Scene::destroyTexture(std::string_view name) {
  textures_.erase(std::string(name));
}

void Scene::destroyCubemap(std::string_view name) {
  cubemaps_.erase(std::string(name));
}

auto Scene::uniformBufferCount() const noexcept -> size_t {
  return uniform_buffers_.size();
}

auto Scene::textureCount() const noexcept -> size_t { return textures_.size(); }

auto Scene::cubemapCount() const noexcept -> size_t { return cubemaps_.size(); }

auto Scene::listUniformBufferNames() const -> std::vector<std::string> {
  return listResourceNames(uniform_buffers_);
}

auto Scene::listTextureNames() const -> std::vector<std::string> {
  return listResourceNames(textures_);
}

auto Scene::listCubemapNames() const -> std::vector<std::string> {
  return listResourceNames(cubemaps_);
}

auto Scene::listMeshNames() const -> std::vector<std::string> {
  return listResourceNames(meshes_);
}

} // namespace vkr::scene
