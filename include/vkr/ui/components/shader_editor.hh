#pragma once

#include "TextEditor.h"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/render/pipeline_library.hh"

namespace vkr::ui {

class ShaderEditor {
public:
  explicit ShaderEditor(render::PipelineLibrary &pipelineLibrary);

  void render();

private:
  // dependencies
  render::PipelineLibrary &pipeline_library_;

  // components
  TextEditor vert_editor_;
  TextEditor frag_editor_;

  // state
  std::string loaded_pipeline_name_{};
  std::string status_message_{};
  bool status_is_error_{false};
  int active_tab_{0};

  // pipeline control
  [[nodiscard]] auto activePipeline() noexcept -> pipeline::GraphicsPipeline *;
  [[nodiscard]] auto hasPipeline() const noexcept -> bool;

  void reloadFromPipeline();
  void reloadFromPipelineIfChanged();
  void applyToPipeline();

  // editor state
  [[nodiscard]] auto currentEditor() noexcept -> TextEditor &;
  [[nodiscard]] auto currentEditor() const noexcept -> const TextEditor &;
  void setStatus(std::string msg, bool isError = false);

  // shader desc helpers
  [[nodiscard]] static auto findShader(pipeline::GraphicsPipelineDesc &desc,
                                       VkShaderStageFlagBits stage)
      -> pipeline::GraphicsShaderStageDesc *;

  [[nodiscard]] static auto
  findShader(const pipeline::GraphicsPipelineDesc &desc,
             VkShaderStageFlagBits stage)
      -> const pipeline::GraphicsShaderStageDesc *;

  [[nodiscard]] static auto
  shaderSource(const pipeline::GraphicsShaderStageDesc *shader) -> std::string;

  [[nodiscard]] static auto
  shaderLabel(const pipeline::GraphicsShaderStageDesc &shader,
              const std::string &fallback) -> std::string;

  [[nodiscard]] static auto
  makeShaderModule(VkShaderStageFlagBits stage, std::string source,
                   std::string label, std::string entryPoint)
      -> resource::ShaderModuleDesc;

  [[nodiscard]] static auto readTextFile(const std::string &path)
      -> std::string;

  // text editor helpers
  [[nodiscard]] static auto makeEditor() -> TextEditor;
  [[nodiscard]] static auto makeGlslLanguageDefinition()
      -> TextEditor::LanguageDefinition;
  [[nodiscard]] static auto materialDarkPalette() -> TextEditor::Palette;
  [[nodiscard]] static auto parseErrors(const std::string &log)
      -> TextEditor::ErrorMarkers;
};

} // namespace vkr::ui
