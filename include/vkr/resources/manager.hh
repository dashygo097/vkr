#pragma once

// buffers
#include "./buffers/buffer_utils.hh"
#include "./buffers/frame_buffer.hh"
#include "./buffers/index_buffer.hh"
#include "./buffers/uniform_buffer.hh"
#include "./buffers/vertex_buffer.hh"

namespace vkr {
class ResourceManager {
public:
  ResourceManager(const VulkanContext &ctx) : ctx(ctx) {}
  ~ResourceManager() = default;

  ResourceManager(const ResourceManager &) = delete;
  ResourceManager &operator=(const ResourceManager &) = delete;

  // Vertex Buffer Management
  template <typename VertexType = Vertex>
  void createVertexBuffer(const std::string &name,
                          const std::vector<VertexType> &vertices) {
    auto vb = std::make_unique<VertexBuffer>(vertices, ctx);
    _vertexBuffers[name] = std::move(vb);
  }

  VertexBuffer *getVertexBuffer(const std::string &name) {
    auto it = _vertexBuffers.find(name);
    return it != _vertexBuffers.end() ? it->second.get() : nullptr;
  }

  void updateVertexBuffer(const std::string &name,
                          const std::vector<Vertex> &newVertices) {
    auto *vb = getVertexBuffer(name);
    if (vb) {
      vb->update(newVertices);
    }
  }

  void destroyVertexBuffer(const std::string &name) {
    _vertexBuffers.erase(name);
  }

  // Index Buffer Management
  void createIndexBuffer(const std::string &name,
                         const std::vector<uint16_t> &indices) {
    auto ib = std::make_unique<IndexBuffer>(indices, ctx);
    _indexBuffers[name] = std::move(ib);
  }

  IndexBuffer *getIndexBuffer(const std::string &name) {
    auto it = _indexBuffers.find(name);
    return it != _indexBuffers.end() ? it->second.get() : nullptr;
  }

  void updateIndexBuffer(const std::string &name,
                         const std::vector<uint16_t> &newIndices) {
    auto *ib = getIndexBuffer(name);
    if (ib) {
      ib->update(newIndices);
    }
  }

  void destroyIndexBuffer(const std::string &name) {
    _indexBuffers.erase(name);
  }

  // Uniform Buffer Management

  // Framebuffer Management
  void createFramebuffers(const std::string &name) {
    auto fb = std::make_unique<Framebuffers>(ctx);
    _framebuffers[name] = std::move(fb);
  }

  void createFramebuffers(const std::string &name, VkRenderPass renderPass,
                          std::vector<VkImageView> swapchainImageViews,
                          VkExtent2D swapchainExtent) {
    auto fb = std::make_unique<Framebuffers>(
        ctx.device, renderPass, swapchainImageViews, swapchainExtent);
    _framebuffers[name] = std::move(fb);
  }

  Framebuffers *getFramebuffers(const std::string &name) {
    auto it = _framebuffers.find(name);
    return it != _framebuffers.end() ? it->second.get() : nullptr;
  }

  void destroyFramebuffers(const std::string &name) {
    _framebuffers.erase(name);
  }

  // Utility functions
  size_t vertexBufferCount() const { return _vertexBuffers.size(); }
  size_t indexBufferCount() const { return _indexBuffers.size(); }
  size_t uniformBufferCount() const { return _uniformBuffers.size(); }

  // List all resources
  std::vector<std::string> listVertexBuffers() const {
    std::vector<std::string> names;
    for (const auto &[name, _] : _vertexBuffers) {
      names.push_back(name);
    }
    return names;
  }

  std::vector<std::string> listIndexBuffers() const {
    std::vector<std::string> names;
    for (const auto &[name, _] : _indexBuffers) {
      names.push_back(name);
    }
    return names;
  }

private:
  VulkanContext ctx;

  std::unordered_map<std::string, std::unique_ptr<VertexBuffer>> _vertexBuffers;
  std::unordered_map<std::string, std::unique_ptr<IndexBuffer>> _indexBuffers;
  std::unordered_map<std::string, std::unique_ptr<DefaultUniformBuffers>>
      _uniformBuffers;
  std::unordered_map<std::string, std::unique_ptr<Framebuffers>> _framebuffers;
};
} // namespace vkr
