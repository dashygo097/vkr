#pragma once
#include <array>
#include <functional>
#include <string>

namespace vkr::ui {

enum class ShaderType { Vertex, Fragment };

struct ShaderEditorState {
  std::array<char, 1024 * 64> vertBuffer{};
  std::array<char, 1024 * 64> fragBuffer{};
  std::string statusMessage{};
  bool statusIsError{false};
};

class ShaderEditor {
public:
  using CompileCallback =
      std::function<void(const std::string &vert, const std::string &frag)>;

  explicit ShaderEditor(CompileCallback onCompile = nullptr)
      : on_compile_(std::move(onCompile)) {}

  void setSource(ShaderType type, const std::string &src) {
    auto &buf =
        type == ShaderType::Vertex ? state_.vertBuffer : state_.fragBuffer;
    std::copy_n(src.begin(), std::min(src.size(), buf.size() - 1), buf.begin());
    buf[std::min(src.size(), buf.size() - 1)] = '\0';
  }

  void setStatus(const std::string &msg, bool isError = false) {
    state_.statusMessage = msg;
    state_.statusIsError = isError;
  }

  void render();

private:
  ShaderEditorState state_;
  CompileCallback on_compile_;
  int active_tab_{0}; // 0 = vert, 1 = frag
};

} // namespace vkr::ui
