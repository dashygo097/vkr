#pragma once

#include <GLFW/glfw3.h>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace vkr::core {

struct WindowDesc {
  std::string title{};
  uint32_t width{};
  uint32_t height{};
  bool resizable{true};

  [[nodiscard]] auto ratio() const noexcept -> float {
    if (height == 0) {
      return 1.0f;
    }

    return static_cast<float>(width) / static_cast<float>(height);
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return !title.empty() && width > 0 && height > 0;
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("title", title);
    ar("width", width);
    ar("height", height);
    ar("resizable", resizable);
  }
};

class Window {
public:
  explicit Window(WindowDesc &desc);
  ~Window();

  Window(const Window &) = delete;
  auto operator=(const Window &) -> Window & = delete;

  [[nodiscard]] auto width() const noexcept -> uint32_t { return desc_.width; }

  [[nodiscard]] auto height() const noexcept -> uint32_t {
    return desc_.height;
  }

  [[nodiscard]] auto shouldClose() const -> bool;
  void pollEvents() const;
  void waitForFramebufferSize();

  [[nodiscard]] auto framebufferResized() const noexcept -> bool {
    return framebuffer_resized_;
  }

  [[nodiscard]] auto consumeFramebufferResized() noexcept -> bool {
    const bool resized = framebuffer_resized_;
    framebuffer_resized_ = false;
    return resized;
  }

  [[nodiscard]] auto glfwWindow() const noexcept -> GLFWwindow * {
    return window_;
  }

private:
  // components
  WindowDesc &desc_;
  GLFWwindow *window_{nullptr};
  bool framebuffer_resized_{false};

  void handleFramebufferResize(int width, int height);
  static auto window(GLFWwindow *glfwWindow) noexcept -> Window *;
  static void framebufferSizeCallback(GLFWwindow *glfwWindow, int width,
                                      int height);

  static std::unordered_map<GLFWwindow *, Window *> windows_;
};

} // namespace vkr::core
