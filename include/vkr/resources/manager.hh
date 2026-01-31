#pragma once

#include "./buffers/frame_buffer.hh"
#include "./buffers/index_buffer.hh"
#include "./buffers/uniform_buffer.hh"
#include "./buffers/vertex_buffer.hh"
#include "./geometry/mesh.hh"
#include "./textures/depth_resources.hh"
#include "./textures/image.hh"
#include "./textures/imageview.hh"
#include "./textures/sampler.hh"
#include <memory>

namespace vkr::resource {

class ResourceManager {
public:
  ResourceManager(const core::Device &device,
                  const core::CommandPool &commandPool,
                  const core::Swapchain &swapchain,
                  const pipeline::RenderPass &renderPass)
      : device_(device), command_pool_(commandPool), swapchain_(swapchain),
        render_pass_(renderPass) {}
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
    auto fb = std::make_shared<Framebuffers>(device_, swapchain_, render_pass_);
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

  // Utility functions
  size_t vertexBufferCount() const { return vertex_buffers_.size(); }
  size_t indexBufferCount() const { return index_buffers_.size(); }
  size_t uniformBufferCount() const { return uniform_buffers_.size(); }
  size_t framebufferCount() const { return frame_buffers_.size(); }

  // List all resources
  std::vector<std::shared_ptr<IVertexBuffer>> listVertexBuffers() const {
    std::vector<std::shared_ptr<IVertexBuffer>> buffers;
    for (const auto &[_, buffer] : vertex_buffers_) {
      buffers.push_back(buffer);
    }
    return buffers;
  }
  std::vector<std::string> listVertexBufferName() const {
    std::vector<std::string> names;
    for (const auto &[name, _] : vertex_buffers_) {
      names.push_back(name);
    }
    return names;
  }

  std::vector<std::shared_ptr<IndexBuffer>> listIndexBuffers() const {
    std::vector<std::shared_ptr<IndexBuffer>> buffers;
    for (const auto &[_, buffer] : index_buffers_) {
      buffers.push_back(buffer);
    }
    return buffers;
  }
  std::vector<std::string> listIndexBufferName() const {
    std::vector<std::string> names;
    for (const auto &[name, _] : index_buffers_) {
      names.push_back(name);
    }
    return names;
  }

  std::vector<std::shared_ptr<IUniformBuffer>> listUniformBuffers() const {
    std::vector<std::shared_ptr<IUniformBuffer>> buffers;
    for (const auto &[_, buffer] : uniform_buffers_) {
      buffers.push_back(buffer);
    }
    return buffers;
  }
  std::vector<std::string> listUniformBufferName() const {
    std::vector<std::string> names;
    for (const auto &[name, _] : uniform_buffers_) {
      names.push_back(name);
    }
    return names;
  }

  std::vector<std::shared_ptr<Framebuffers>> listFramebuffers() const {
    std::vector<std::shared_ptr<Framebuffers>> buffers;
    for (const auto &[_, buffer] : frame_buffers_) {
      buffers.push_back(buffer);
    }
    return buffers;
  }
  std::vector<std::string> listFramebufferName() const {
    std::vector<std::string> names;
    for (const auto &[name, _] : frame_buffers_) {
      names.push_back(name);
    }
    return names;
  }

private:
  // dependencies
  const core::Device &device_;
  const core::CommandPool &command_pool_;
  const core::Swapchain &swapchain_;
  const pipeline::RenderPass &render_pass_;

  // components
  std::unordered_map<std::string, std::shared_ptr<IVertexBuffer>>
      vertex_buffers_;
  std::unordered_map<std::string, std::shared_ptr<IndexBuffer>> index_buffers_;
  std::unordered_map<std::string, std::shared_ptr<IUniformBuffer>>
      uniform_buffers_;
  std::unordered_map<std::string, std::shared_ptr<Framebuffers>> frame_buffers_;
};
} // namespace vkr::resource
