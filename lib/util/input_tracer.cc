#include "vkr/util/input_tracer.hh"

namespace vkr::util {

std::unordered_map<GLFWwindow *, InputTracer *> InputTracer::tracers_{};

InputTracer::InputTracer(GLFWwindow *window) : window_(window) {
  if (window_) {
    glfwGetCursorPos(window_, &cursor_position_.x, &cursor_position_.y);
    previous_cursor_position_ = cursor_position_;
  }
}

InputTracer::~InputTracer() { restoreCallbacks(); }

void InputTracer::installCallbacks() {
  if (!window_ || callbacks_installed_) {
    return;
  }

  tracers_[window_] = this;
  previous_key_callback_ = glfwSetKeyCallback(window_, keyCallback);
  previous_char_callback_ = glfwSetCharCallback(window_, charCallback);
  previous_cursor_position_callback_ =
      glfwSetCursorPosCallback(window_, cursorPositionCallback);
  previous_mouse_button_callback_ =
      glfwSetMouseButtonCallback(window_, mouseButtonCallback);
  previous_scroll_callback_ = glfwSetScrollCallback(window_, scrollCallback);
  callbacks_installed_ = true;
}

void InputTracer::beginFrame() {
  previous_keys_ = keys_;
  previous_mouse_buttons_ = mouse_buttons_;
  previous_cursor_position_ = cursor_position_;

  cursor_delta_ = {};
  scroll_offset_ = {};
  characters_.clear();
}

void InputTracer::update() {
  if (!window_) {
    return;
  }

  glfwGetCursorPos(window_, &cursor_position_.x, &cursor_position_.y);
  cursor_delta_.x = cursor_position_.x - previous_cursor_position_.x;
  cursor_delta_.y = cursor_position_.y - previous_cursor_position_.y;
}

auto InputTracer::isKeyDown(int key) const noexcept -> bool {
  if (!validKey(key)) {
    return false;
  }

  if (window_) {
    return glfwGetKey(window_, key) == GLFW_PRESS;
  }

  return keys_[static_cast<size_t>(key)];
}

auto InputTracer::wasKeyPressed(int key) const noexcept -> bool {
  if (!validKey(key)) {
    return false;
  }

  const auto index = static_cast<size_t>(key);
  return keys_[index] && !previous_keys_[index];
}

auto InputTracer::wasKeyReleased(int key) const noexcept -> bool {
  if (!validKey(key)) {
    return false;
  }

  const auto index = static_cast<size_t>(key);
  return !keys_[index] && previous_keys_[index];
}

auto InputTracer::isMouseButtonDown(int button) const noexcept -> bool {
  if (!validMouseButton(button)) {
    return false;
  }

  if (window_) {
    return glfwGetMouseButton(window_, button) == GLFW_PRESS;
  }

  return mouse_buttons_[static_cast<size_t>(button)];
}

auto InputTracer::wasMouseButtonPressed(int button) const noexcept -> bool {
  if (!validMouseButton(button)) {
    return false;
  }

  const auto index = static_cast<size_t>(button);
  return mouse_buttons_[index] && !previous_mouse_buttons_[index];
}

auto InputTracer::wasMouseButtonReleased(int button) const noexcept -> bool {
  if (!validMouseButton(button)) {
    return false;
  }

  const auto index = static_cast<size_t>(button);
  return !mouse_buttons_[index] && previous_mouse_buttons_[index];
}

void InputTracer::handleKey(int key, int action) {
  if (!validKey(key)) {
    return;
  }

  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    keys_[static_cast<size_t>(key)] = true;
  } else if (action == GLFW_RELEASE) {
    keys_[static_cast<size_t>(key)] = false;
  }
}

void InputTracer::handleCharacter(unsigned int codepoint) {
  characters_.push_back(codepoint);
}

void InputTracer::handleCursorPosition(double x, double y) {
  cursor_position_ = {.x = x, .y = y};
}

void InputTracer::handleMouseButton(int button, int action) {
  if (!validMouseButton(button)) {
    return;
  }

  if (action == GLFW_PRESS) {
    mouse_buttons_[static_cast<size_t>(button)] = true;
  } else if (action == GLFW_RELEASE) {
    mouse_buttons_[static_cast<size_t>(button)] = false;
  }
}

void InputTracer::handleScroll(double xOffset, double yOffset) {
  scroll_offset_.x += xOffset;
  scroll_offset_.y += yOffset;
}

void InputTracer::restoreCallbacks() {
  if (!window_ || !callbacks_installed_) {
    return;
  }

  glfwSetKeyCallback(window_, previous_key_callback_);
  glfwSetCharCallback(window_, previous_char_callback_);
  glfwSetCursorPosCallback(window_, previous_cursor_position_callback_);
  glfwSetMouseButtonCallback(window_, previous_mouse_button_callback_);
  glfwSetScrollCallback(window_, previous_scroll_callback_);

  tracers_.erase(window_);
  callbacks_installed_ = false;
}

auto InputTracer::validKey(int key) noexcept -> bool {
  return key >= 0 && key <= GLFW_KEY_LAST;
}

auto InputTracer::validMouseButton(int button) noexcept -> bool {
  return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST;
}

auto InputTracer::tracer(GLFWwindow *window) noexcept -> InputTracer * {
  const auto it = tracers_.find(window);
  if (it == tracers_.end()) {
    return nullptr;
  }

  return it->second;
}

void InputTracer::keyCallback(GLFWwindow *window, int key, int scancode,
                              int action, int mods) {
  auto *input = tracer(window);
  if (!input) {
    return;
  }

  input->handleKey(key, action);

  if (input->previous_key_callback_) {
    input->previous_key_callback_(window, key, scancode, action, mods);
  }
}

void InputTracer::charCallback(GLFWwindow *window, unsigned int codepoint) {
  auto *input = tracer(window);
  if (!input) {
    return;
  }

  input->handleCharacter(codepoint);

  if (input->previous_char_callback_) {
    input->previous_char_callback_(window, codepoint);
  }
}

void InputTracer::cursorPositionCallback(GLFWwindow *window, double x,
                                         double y) {
  auto *input = tracer(window);
  if (!input) {
    return;
  }

  input->handleCursorPosition(x, y);

  if (input->previous_cursor_position_callback_) {
    input->previous_cursor_position_callback_(window, x, y);
  }
}

void InputTracer::mouseButtonCallback(GLFWwindow *window, int button,
                                      int action, int mods) {
  auto *input = tracer(window);
  if (!input) {
    return;
  }

  input->handleMouseButton(button, action);

  if (input->previous_mouse_button_callback_) {
    input->previous_mouse_button_callback_(window, button, action, mods);
  }
}

void InputTracer::scrollCallback(GLFWwindow *window, double xOffset,
                                 double yOffset) {
  auto *input = tracer(window);
  if (!input) {
    return;
  }

  input->handleScroll(xOffset, yOffset);

  if (input->previous_scroll_callback_) {
    input->previous_scroll_callback_(window, xOffset, yOffset);
  }
}

} // namespace vkr::util
