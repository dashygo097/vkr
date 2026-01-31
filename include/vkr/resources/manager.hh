#pragma once

#include "./buffers/frame_buffer.hh"
#include "./buffers/index_buffer.hh"
#include "./buffers/uniform_buffer.hh"
#include "./buffers/vertex_buffer.hh"
#include "./depth_resources.hh"
#include "./mesh.hh"
#include "./textures/image.hh"
#include "./textures/imageview.hh"
#include "./textures/sampler.hh"
#include <memory>
#include <vector>

namespace vkr::resource {

class ResourceManager {
public:
  ResourceManager(const core::Device &device, const core::Swapchain &swapchain,
                  const core::CommandPool &commandPool,
                  const DepthResources &depthResources,
                  const pipeline::RenderPass &renderPass)
      : device_(device), swapchain_(swapchain), command_pool_(commandPool),
        depth_resources_(depthResources), render_pass_(renderPass) {}
  ~ResourceManager() = default;

  ResourceManager(const ResourceManager &) = delete;
  ResourceManager &operator=(const ResourceManager &) = delete;

  // Vertex Buffer Management
  template <typename VBOType>
  void createVertexBuffer(const std::string &name,
                          const std::vector<VBOType> &vertices) {
    auto vb =
        std::make_shared<VertexBufferBase<VBOType>>(device_, command_pool_);
    vb->update(vertices);
    vertex_buffers_[name] = std::move(vb);
  }

  std::shared_ptr<IVertexBuffer> getVertexBuffer(const std::string &name) {
    auto it = vertex_buffers_.find(name);
    return it != vertex_buffers_.end() ? it->second : nullptr;
  }

  void destroyVertexBuffer(const std::string &name) {
    vertex_buffers_.erase(name);
  }

  // Index Buffer Management
  void createIndexBuffer(const std::string &name,
                         const std::vector<uint16_t> &indices) {
    auto ib = std::make_shared<IndexBuffer>(device_, command_pool_);
    ib->update(indices);
    index_buffers_[name] = std::move(ib);
  }

  std::shared_ptr<IndexBuffer> getIndexBuffer(const std::string &name) {
    auto it = index_buffers_.find(name);
    return it != index_buffers_.end() ? it->second : nullptr;
  }

  void destroyIndexBuffer(const std::string &name) {
    index_buffers_.erase(name);
  }

  // Uniform Buffer Management
  template <typename UBOType>
  void createUniformBuffer(const std::string &name, const UBOType &ubo) {
    auto ub = std::make_shared<UniformBufferBase<UBOType>>(device_);
    ub->update(0, ubo);
    uniform_buffers_[name] = std::move(ub);
  }

  std::shared_ptr<IUniformBuffer> getUniformBuffer(const std::string &name) {
    auto it = uniform_buffers_.find(name);
    return it != uniform_buffers_.end() ? it->second : nullptr;
  }

  void destroyUniformBuffer(const std::string &name) {
    uniform_buffers_.erase(name);
  }

  // Framebuffer Management
  void createFramebuffers(const std::string &name) {
    auto fb = std::make_shared<Framebuffers>(device_, swapchain_,
                                             depth_resources_, render_pass_);
    frame_buffers_[name] = std::move(fb);
  }

  std::shared_ptr<Framebuffers> getFramebuffers(const std::string &name) {
    auto it = frame_buffers_.find(name);
    return it != frame_buffers_.end() ? it->second : nullptr;
  }

  void destroyFramebuffers(const std::string &name) {
    frame_buffers_.erase(name);
  }

  // Mesh Management
  template <typename VBOType>
  void createMesh(const std::string &name, const Mesh<VBOType> &mesh) {
    createVertexBuffer<VBOType>(name, mesh.vertexBuffer()->vertices());
    createIndexBuffer(name, mesh.indexBuffer()->indices());
  }

  // Texture Management
  void createTexture(const std::string &name, const std::string &filePath) {
    auto image = std::make_shared<Image>(device_, command_pool_);
    auto imageview = std::make_shared<ImageView>(device_);
    auto sampler = std::make_shared<Sampler>(device_);
    image->create(filePath);
    imageview->create(*image, VK_FORMAT_R8G8B8A8_SRGB);
    texture_images_[name] = std::move(image);
    texture_imageviews_[name] = std::move(imageview);
    texture_samplers_[name] = std::move(sampler);
  }

  std::shared_ptr<Image> getTextureImage(const std::string &name) {
    auto it = texture_images_.find(name);
    return it != texture_images_.end() ? it->second : nullptr;
  }

  std::shared_ptr<ImageView> getTextureImageView(const std::string &name) {
    auto it = texture_imageviews_.find(name);
    return it != texture_imageviews_.end() ? it->second : nullptr;
  }

  std::shared_ptr<Sampler> getTextureSampler(const std::string &name) {
    auto it = texture_samplers_.find(name);
    return it != texture_samplers_.end() ? it->second : nullptr;
  }

  void destroyTexture(const std::string &name) {
    texture_samplers_.erase(name);
    texture_imageviews_.erase(name);
    texture_images_.erase(name);
  }

  // Utility functions
  size_t vertexBufferCount() const { return vertex_buffers_.size(); }
  size_t indexBufferCount() const { return index_buffers_.size(); }
  size_t uniformBufferCount() const { return uniform_buffers_.size(); }
  size_t framebufferCount() const { return frame_buffers_.size(); }
  size_t textureImageCount() const { return texture_images_.size(); }

  // List all resources
  template <typename ResourceType>
  std::vector<std::shared_ptr<ResourceType>> listResources(
      const std::unordered_map<std::string, std::shared_ptr<ResourceType>>
          &resource_map) const {
    std::vector<std::shared_ptr<ResourceType>> resources;
    for (const auto &[_, resource] : resource_map) {
      resources.push_back(resource);
    }
    return resources;
  }
  template <typename ResourceType>
  std::vector<std::string> listResourceNames(
      const std::unordered_map<std::string, std::shared_ptr<ResourceType>>
          &resource_map) const {
    std::vector<std::string> names;
    for (const auto &[name, _] : resource_map) {
      names.push_back(name);
    }
    return names;
  }

  std::vector<std::shared_ptr<IVertexBuffer>> listVertexBuffers() const {
    return listResources<IVertexBuffer>(vertex_buffers_);
  }
  std::vector<std::string> listVertexBufferNames() const {
    return listResourceNames<IVertexBuffer>(vertex_buffers_);
  }
  std::vector<std::shared_ptr<IndexBuffer>> listIndexBuffers() const {
    return listResources<IndexBuffer>(index_buffers_);
  }
  std::vector<std::string> listIndexBufferNames() const {
    return listResourceNames<IndexBuffer>(index_buffers_);
  }
  std::vector<std::shared_ptr<IUniformBuffer>> listUniformBuffers() const {
    return listResources<IUniformBuffer>(uniform_buffers_);
  }
  std::vector<std::string> listUniformBufferNames() const {
    return listResourceNames<IUniformBuffer>(uniform_buffers_);
  }
  std::vector<std::shared_ptr<Framebuffers>> listFramebuffers() const {
    return listResources<Framebuffers>(frame_buffers_);
  }
  std::vector<std::string> listFramebufferNames() const {
    return listResourceNames<Framebuffers>(frame_buffers_);
  }
  std::vector<std::shared_ptr<Image>> listTextureImages() const {
    return listResources<Image>(texture_images_);
  }
  std::vector<std::string> listTextureImageNames() const {
    return listResourceNames<Image>(texture_images_);
  }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  const core::Swapchain &swapchain_;
  const DepthResources &depth_resources_;
  const pipeline::RenderPass &render_pass_;

  // components
  std::unordered_map<std::string, std::shared_ptr<IVertexBuffer>>
      vertex_buffers_;
  std::unordered_map<std::string, std::shared_ptr<IndexBuffer>> index_buffers_;
  std::unordered_map<std::string, std::shared_ptr<IUniformBuffer>>
      uniform_buffers_;
  std::unordered_map<std::string, std::shared_ptr<Framebuffers>> frame_buffers_;
  std::unordered_map<std::string, std::shared_ptr<Image>> texture_images_;
  std::unordered_map<std::string, std::shared_ptr<ImageView>>
      texture_imageviews_;
  std::unordered_map<std::string, std::shared_ptr<Sampler>> texture_samplers_;
};
} // namespace vkr::resource
