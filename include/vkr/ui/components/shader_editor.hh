#pragma once

#include "TextEditor.h"
#include "vkr/exec/render/graph.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include "vkr/ui/components/ui_component.hh"
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vkr::ui {

struct ShaderEditorFileState {
  std::string path{};
  std::filesystem::file_time_type write_time{};
  std::string disk_text{};
  std::string synced_text{};
  std::string valid_text{};
  bool file_backed{false};
  bool conflict{false};
};

struct ShaderEditorPipelineState {
  ShaderEditorFileState vert{};
  ShaderEditorFileState frag{};
};

class ShaderEditor final : public UiComponent {
public:
  explicit ShaderEditor(exec::RenderGraph &graph);

private:
  void render();

  struct PipelineTarget {
    std::string passName{};
    std::string pipelineName{};
    std::reference_wrapper<pipeline::GraphicsPipeline> pipeline;

    [[nodiscard]] auto label() const -> std::string {
      return passName + " / " + pipelineName;
    }
  };

  // dependencies
  exec::RenderGraph &graph_;

  // components
  TextEditor vert_editor_;
  TextEditor frag_editor_;

  // state
  std::string selected_pass_name_{};
  std::string loaded_target_key_{};
  std::string status_message_{};
  bool status_is_error_{false};
  int active_tab_{0};
  double last_file_probe_time_{0.0};
  ShaderEditorFileState vert_file_{};
  ShaderEditorFileState frag_file_{};
  std::unordered_map<std::string, ShaderEditorPipelineState> pipeline_files_{};

  // pipeline control
  [[nodiscard]] auto collectTargets() -> std::vector<PipelineTarget>;
  [[nodiscard]] auto activeTarget() -> std::optional<PipelineTarget>;
  [[nodiscard]] auto activePipeline()
      -> std::optional<std::reference_wrapper<pipeline::GraphicsPipeline>>;
  [[nodiscard]] auto hasPipeline() const -> bool;
  [[nodiscard]] auto activeTargetKey() -> std::string;

  void reloadFromPipeline();
  void reloadFromPipelineIfChanged();
  void refreshFileBackedShaders(const std::vector<PipelineTarget> &targets);
  [[nodiscard]] auto applyToPipeline(bool saveFiles = true) -> bool;
  void renderTargetSelector(const std::vector<PipelineTarget> &targets);

  // editor state
  [[nodiscard]] auto currentEditor() noexcept -> TextEditor &;
  [[nodiscard]] auto currentEditor() const noexcept -> const TextEditor &;
  [[nodiscard]] auto currentFileState() noexcept -> ShaderEditorFileState &;
  [[nodiscard]] auto currentFileState() const noexcept
      -> const ShaderEditorFileState &;
  void setStatus(std::string msg, bool isError = false);

  // shader desc helpers
  [[nodiscard]] static auto findShader(pipeline::GraphicsPipelineDesc &desc,
                                       VkShaderStageFlagBits stage)
      -> std::optional<
          std::reference_wrapper<pipeline::GraphicsShaderStageDesc>>;

  [[nodiscard]] static auto
  findShader(const pipeline::GraphicsPipelineDesc &desc,
             VkShaderStageFlagBits stage)
      -> std::optional<
          std::reference_wrapper<const pipeline::GraphicsShaderStageDesc>>;

  [[nodiscard]] static auto
  shaderSource(std::optional<
               std::reference_wrapper<const pipeline::GraphicsShaderStageDesc>>
                   shader) -> std::string;

  [[nodiscard]] static auto
  shaderLabel(const pipeline::GraphicsShaderStageDesc &shader,
              const std::string &fallback) -> std::string;

  [[nodiscard]] static auto
  shaderPath(std::optional<
             std::reference_wrapper<const pipeline::GraphicsShaderStageDesc>>
                 shader) -> std::string;

  [[nodiscard]] static auto
  makeShaderModule(VkShaderStageFlagBits stage, std::string source,
                   std::string label, std::string entryPoint, bool fileBacked)
      -> resource::ShaderModuleDesc;

  [[nodiscard]] static auto readTextFile(const std::string &path)
      -> std::string;
  [[nodiscard]] static auto writeTextFile(const std::string &path,
                                          const std::string &text) -> bool;
  [[nodiscard]] static auto fileWriteTime(const std::string &path)
      -> std::optional<std::filesystem::file_time_type>;

  // text editor helpers
  [[nodiscard]] static auto makeEditor() -> TextEditor;
  [[nodiscard]] static auto makeGlslLanguageDefinition()
      -> TextEditor::LanguageDefinition;
  [[nodiscard]] static auto materialDarkPalette() -> TextEditor::Palette;
  [[nodiscard]] static auto parseErrors(const std::string &log)
      -> TextEditor::ErrorMarkers;
};

} // namespace vkr::ui
