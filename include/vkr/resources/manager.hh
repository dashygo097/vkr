#pragma once

// buffers
#include "./buffers/buffer_utils.hh"
#include "./buffers/frame_buffer.hh"
#include "./buffers/index_buffer.hh"
#include "./buffers/uniform_buffer.hh"
#include "./buffers/vertex_buffer.hh"

namespace vkr {

template <typename UBOType> class ResourceManager {
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

  void destroyIndexBuffer(const std::string &name) {
    _indexBuffers.erase(name);
  }

  // Uniform Buffer Management

  // Framebuffer Management
  void createFramebuffers(const std::string &name) {
    auto fb = std::make_unique<Framebuffers>(ctx);
    _framebuffers[name] = std::move(fb);
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
  UBOType uboType;

  std::unordered_map<std::string, std::unique_ptr<VertexBuffer>> _vertexBuffers;
  std::unordered_map<std::string, std::unique_ptr<IndexBuffer>> _indexBuffers;
  std::unordered_map<std::string, std::unique_ptr<UniformBufferBase<UBOType>>>
      _uniformBuffers;
  std::unordered_map<std::string, std::unique_ptr<Framebuffers>> _framebuffers;
};
} // namespace vkr
