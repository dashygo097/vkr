#pragma once

#include <GLFW/glfw3.h>
#include <array>
#include <unordered_map>
#include <vector>

namespace vkr::util {

struct InputPoint {
  double x{0.0};
  double y{0.0};
};

class InputTracer {
public:
  explicit InputTracer(GLFWwindow *window);
  ~InputTracer();

  InputTracer(const InputTracer &) = delete;
  auto operator=(const InputTracer &) -> InputTracer & = delete;

  void installCallbacks();
  void beginFrame();
  void update();

  [[nodiscard]] auto isKeyDown(int key) const noexcept -> bool;
  [[nodiscard]] auto wasKeyPressed(int key) const noexcept -> bool;
  [[nodiscard]] auto wasKeyReleased(int key) const noexcept -> bool;

  [[nodiscard]] auto isMouseButtonDown(int button) const noexcept -> bool;
  [[nodiscard]] auto wasMouseButtonPressed(int button) const noexcept -> bool;
  [[nodiscard]] auto wasMouseButtonReleased(int button) const noexcept -> bool;

  [[nodiscard]] auto cursorPosition() const noexcept -> InputPoint {
    return cursor_position_;
  }

  [[nodiscard]] auto cursorDelta() const noexcept -> InputPoint {
    return cursor_delta_;
  }

  [[nodiscard]] auto scrollOffset() const noexcept -> InputPoint {
    return scroll_offset_;
  }

  [[nodiscard]] auto characters() const noexcept
      -> const std::vector<unsigned int> & {
    return characters_;
  }

private:
  static constexpr size_t KeyCount = GLFW_KEY_LAST + 1;
  static constexpr size_t MouseButtonCount = GLFW_MOUSE_BUTTON_LAST + 1;

  GLFWwindow *window_{nullptr};

  std::array<bool, KeyCount> keys_{};
  std::array<bool, KeyCount> previous_keys_{};
  std::array<bool, MouseButtonCount> mouse_buttons_{};
  std::array<bool, MouseButtonCount> previous_mouse_buttons_{};
  std::vector<unsigned int> characters_{};

  InputPoint cursor_position_{};
  InputPoint previous_cursor_position_{};
  InputPoint cursor_delta_{};
  InputPoint scroll_offset_{};

  GLFWkeyfun previous_key_callback_{nullptr};
  GLFWcharfun previous_char_callback_{nullptr};
  GLFWcursorposfun previous_cursor_position_callback_{nullptr};
  GLFWmousebuttonfun previous_mouse_button_callback_{nullptr};
  GLFWscrollfun previous_scroll_callback_{nullptr};
  bool callbacks_installed_{false};

  void handleKey(int key, int action);
  void handleCharacter(unsigned int codepoint);
  void handleCursorPosition(double x, double y);
  void handleMouseButton(int button, int action);
  void handleScroll(double xOffset, double yOffset);
  void restoreCallbacks();

  [[nodiscard]] static auto validKey(int key) noexcept -> bool;
  [[nodiscard]] static auto validMouseButton(int button) noexcept -> bool;

  static auto tracer(GLFWwindow *window) noexcept -> InputTracer *;
  static void keyCallback(GLFWwindow *window, int key, int scancode, int action,
                          int mods);
  static void charCallback(GLFWwindow *window, unsigned int codepoint);
  static void cursorPositionCallback(GLFWwindow *window, double x, double y);
  static void mouseButtonCallback(GLFWwindow *window, int button, int action,
                                  int mods);
  static void scrollCallback(GLFWwindow *window, double xOffset,
                             double yOffset);

  static std::unordered_map<GLFWwindow *, InputTracer *> tracers_;
};

} // namespace vkr::util
