#pragma once

#include <GLFW/glfw3.h>
#include <cstdint>
#include <string>

namespace vkr::core {

struct WindowDesc {
  std::string title{};
  uint32_t width{};
  uint32_t height{};

  [[nodiscard]] auto ratio() const noexcept -> float {
    return static_cast<float>(width) / static_cast<float>(height);
  }

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return !title.empty() && width > 0 && height > 0;
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("title", title);
    ar("width", width);
    ar("height", height);
  }
};

class Window {
public:
  explicit Window(WindowDesc &desc);
  ~Window();

  Window(const Window &) = delete;
  auto operator=(const Window &) -> Window & = delete;

  [[nodiscard]] auto desc() const noexcept -> const WindowDesc & {
    return desc_;
  }

  [[nodiscard]] auto shouldClose() const -> bool;
  void pollEvents() const;

  [[nodiscard]] auto glfwWindow() const noexcept -> GLFWwindow * {
    return window_;
  }

private:
  // components
  GLFWwindow *window_{nullptr};
  WindowDesc &desc_;
};

} // namespace vkr::core
