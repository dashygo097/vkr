#pragma once

#include "./buffers/frame_buffer.hh"
#include "./buffers/index_buffer.hh"
#include "./buffers/uniform_buffer.hh"
#include "./buffers/vertex_buffer.hh"
#include "./geometry/mesh.hh"
#include "./textures/depth_resources.hh"
#include "./textures/image.hh"
#include "./textures/image_view.hh"
#include "./textures/sampler.hh"
#include <memory>

namespace vkr::resource {

class ResourceManager {
public:
  ResourceManager(const core::Device &device,
                  const core::CommandPool &commandPool,
                  const core::Swapchain &swapchain,
                  const pipeline::RenderPass &renderPass)
      : device(device), commandPool(commandPool), swapchain(swapchain),
        renderPass(renderPass) {}
  ~ResourceManager() = default;

  ResourceManager(const ResourceManager &) = delete;
  ResourceManager &operator=(const ResourceManager &) = delete;

  // Vertex Buffer Management
  template <typename VBOType>
  void createVertexBuffer(const std::string &name,
                          const std::vector<VBOType> &vertices) {
    auto vb = std::make_shared<VertexBufferBase<VBOType>>(device, commandPool);
    vb->update(vertices);
    _vertexBuffers[name] = std::move(vb);
  }

  std::shared_ptr<IVertexBuffer> getVertexBuffer(const std::string &name) {
    auto it = _vertexBuffers.find(name);
    return it != _vertexBuffers.end() ? it->second : nullptr;
  }

  void destroyVertexBuffer(const std::string &name) {
    _vertexBuffers.erase(name);
  }

  // Index Buffer Management
  void createIndexBuffer(const std::string &name,
                         const std::vector<uint16_t> &indices) {
    auto ib = std::make_shared<IndexBuffer>(device, commandPool);
    ib->update(indices);
    _indexBuffers[name] = std::move(ib);
  }

  std::shared_ptr<IndexBuffer> getIndexBuffer(const std::string &name) {
    auto it = _indexBuffers.find(name);
    return it != _indexBuffers.end() ? it->second : nullptr;
  }

  void destroyIndexBuffer(const std::string &name) {
    _indexBuffers.erase(name);
  }

  // Uniform Buffer Management
  template <typename UBOType>
  void createUniformBuffer(const std::string &name, const UBOType &ubo) {
    auto ub = std::make_shared<UniformBufferBase<UBOType>>(device);
    ub->update(0, ubo);
    _uniformBuffers[name] = std::move(ub);
  }

  std::shared_ptr<IUniformBuffer> getUniformBuffer(const std::string &name) {
    auto it = _uniformBuffers.find(name);
    return it != _uniformBuffers.end() ? it->second : nullptr;
  }

  void destroyUniformBuffer(const std::string &name) {
    _uniformBuffers.erase(name);
  }

  // Framebuffer Management
  void createFramebuffers(const std::string &name) {
    auto fb = std::make_shared<Framebuffers>(device, swapchain, renderPass);
    _framebuffers[name] = std::move(fb);
  }

  std::shared_ptr<Framebuffers> getFramebuffers(const std::string &name) {
    auto it = _framebuffers.find(name);
    return it != _framebuffers.end() ? it->second : nullptr;
  }

  void destroyFramebuffers(const std::string &name) {
    _framebuffers.erase(name);
  }

  // Utility functions
  size_t vertexBufferCount() const { return _vertexBuffers.size(); }
  size_t indexBufferCount() const { return _indexBuffers.size(); }
  size_t uniformBufferCount() const { return _uniformBuffers.size(); }
  size_t framebufferCount() const { return _framebuffers.size(); }

  // List all resources
  std::vector<std::shared_ptr<IVertexBuffer>> listVertexBuffers() const {
    std::vector<std::shared_ptr<IVertexBuffer>> buffers;
    for (const auto &[_, buffer] : _vertexBuffers) {
      buffers.push_back(buffer);
    }
    return buffers;
  }
  std::vector<std::string> listVertexBufferName() const {
    std::vector<std::string> names;
    for (const auto &[name, _] : _vertexBuffers) {
      names.push_back(name);
    }
    return names;
  }

  std::vector<std::shared_ptr<IndexBuffer>> listIndexBuffers() const {
    std::vector<std::shared_ptr<IndexBuffer>> buffers;
    for (const auto &[_, buffer] : _indexBuffers) {
      buffers.push_back(buffer);
    }
    return buffers;
  }
  std::vector<std::string> listIndexBufferName() const {
    std::vector<std::string> names;
    for (const auto &[name, _] : _indexBuffers) {
      names.push_back(name);
    }
    return names;
  }

  std::vector<std::shared_ptr<IUniformBuffer>> listUniformBuffers() const {
    std::vector<std::shared_ptr<IUniformBuffer>> buffers;
    for (const auto &[_, buffer] : _uniformBuffers) {
      buffers.push_back(buffer);
    }
    return buffers;
  }
  std::vector<std::string> listUniformBufferName() const {
    std::vector<std::string> names;
    for (const auto &[name, _] : _uniformBuffers) {
      names.push_back(name);
    }
    return names;
  }

  std::vector<std::shared_ptr<Framebuffers>> listFramebuffers() const {
    std::vector<std::shared_ptr<Framebuffers>> buffers;
    for (const auto &[_, buffer] : _framebuffers) {
      buffers.push_back(buffer);
    }
    return buffers;
  }
  std::vector<std::string> listFramebufferName() const {
    std::vector<std::string> names;
    for (const auto &[name, _] : _framebuffers) {
      names.push_back(name);
    }
    return names;
  }

private:
  // dependencies
  const core::Device &device;
  const core::CommandPool &commandPool;
  const core::Swapchain &swapchain;
  const pipeline::RenderPass &renderPass;

  // components
  std::unordered_map<std::string, std::shared_ptr<IVertexBuffer>>
      _vertexBuffers;
  std::unordered_map<std::string, std::shared_ptr<IndexBuffer>> _indexBuffers;
  std::unordered_map<std::string, std::shared_ptr<IUniformBuffer>>
      _uniformBuffers;
  std::unordered_map<std::string, std::shared_ptr<Framebuffers>> _framebuffers;
};
} // namespace vkr::resource
