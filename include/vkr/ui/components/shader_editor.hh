#pragma once

#include "TextEditor.h"
#include <functional>
#include <string>

namespace vkr::ui {

enum class ShaderType { Vertex, Fragment };

class ShaderEditor {
public:
  using CompileCallback =
      std::function<void(const std::string &vert, const std::string &frag)>;

  explicit ShaderEditor(CompileCallback onCompile = nullptr);

  void setSource(ShaderType type, const std::string &src);
  void setStatus(const std::string &msg, bool isError = false);
  void render();

private:
  TextEditor vert_editor_;
  TextEditor frag_editor_;

  std::string status_message_;
  bool status_is_error_{false};

  CompileCallback on_compile_;
  int active_tab_{0}; // 0 = vert, 1 = frag

  // helpers
  static auto makeEditor() -> TextEditor;
  static auto materialDarkPalette() -> TextEditor::Palette;
  static auto parseErrors(const std::string &log) -> TextEditor::ErrorMarkers;
};

} // namespace vkr::ui
