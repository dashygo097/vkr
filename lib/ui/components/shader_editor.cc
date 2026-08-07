#include "vkr/ui/components/shader_editor.hh"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <initializer_list>
#include <sstream>
#include <utility>

namespace vkr::ui {

static auto abgr(uint8_t r, uint8_t g, uint8_t b, uint8_t a) -> uint32_t {
  return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) |
         (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(r);
}

static auto styleColorToAbgr(const ImVec4 &color) -> uint32_t {
  auto toByte = [](float value) -> uint8_t {
    return static_cast<uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
  };

  return abgr(toByte(color.x), toByte(color.y), toByte(color.z),
              toByte(color.w));
}

static auto styleColorToAbgr(const ImVec4 &color, float alpha) -> uint32_t {
  ImVec4 c = color;
  c.w = alpha;
  return styleColorToAbgr(c);
}

static auto relativeLuminance(const ImVec4 &color) -> float {
  return color.x * 0.2126f + color.y * 0.7152f + color.z * 0.0722f;
}

static void addKeywords(TextEditor::LanguageDefinition &lang,
                        std::initializer_list<const char *> words) {
  for (const char *word : words) {
    lang.mKeywords.insert(word);
  }
}

static void addIdentifiers(TextEditor::LanguageDefinition &lang,
                           std::initializer_list<const char *> words,
                           const std::string &declaration) {
  for (const char *word : words) {
    TextEditor::Identifier identifier;
    identifier.mDeclaration = declaration;
    lang.mIdentifiers.insert_or_assign(word, identifier);
  }
}

static void addPreprocIdentifiers(TextEditor::LanguageDefinition &lang,
                                  std::initializer_list<const char *> words,
                                  const std::string &declaration) {
  for (const char *word : words) {
    TextEditor::Identifier identifier;
    identifier.mDeclaration = declaration;
    lang.mPreprocIdentifiers.insert_or_assign(word, identifier);
  }
}

auto ShaderEditor::makeGlslLanguageDefinition()
    -> TextEditor::LanguageDefinition {
  auto lang = TextEditor::LanguageDefinition::GLSL();

  addKeywords(lang, {
                        "attribute",
                        "const",
                        "uniform",
                        "varying",
                        "buffer",
                        "shared",
                        "coherent",
                        "volatile",
                        "restrict",
                        "readonly",
                        "writeonly",
                        "atomic_uint",
                        "layout",
                        "centroid",
                        "flat",
                        "smooth",
                        "noperspective",
                        "patch",
                        "sample",
                        "invariant",
                        "precise",
                        "break",
                        "continue",
                        "do",
                        "for",
                        "while",
                        "switch",
                        "case",
                        "default",
                        "if",
                        "else",
                        "subroutine",
                        "in",
                        "out",
                        "inout",
                        "true",
                        "false",
                        "discard",
                        "return",
                        "lowp",
                        "mediump",
                        "highp",
                        "precision",
                        "struct",
                        "void",
                        "bool",
                        "int",
                        "uint",
                        "float",
                        "double",
                        "bvec2",
                        "bvec3",
                        "bvec4",
                        "ivec2",
                        "ivec3",
                        "ivec4",
                        "uvec2",
                        "uvec3",
                        "uvec4",
                        "vec2",
                        "vec3",
                        "vec4",
                        "dvec2",
                        "dvec3",
                        "dvec4",
                        "mat2",
                        "mat3",
                        "mat4",
                        "mat2x2",
                        "mat2x3",
                        "mat2x4",
                        "mat3x2",
                        "mat3x3",
                        "mat3x4",
                        "mat4x2",
                        "mat4x3",
                        "mat4x4",
                        "dmat2",
                        "dmat3",
                        "dmat4",
                        "dmat2x2",
                        "dmat2x3",
                        "dmat2x4",
                        "dmat3x2",
                        "dmat3x3",
                        "dmat3x4",
                        "dmat4x2",
                        "dmat4x3",
                        "dmat4x4",
                        "sampler1D",
                        "sampler2D",
                        "sampler3D",
                        "samplerCube",
                        "sampler2DRect",
                        "sampler1DArray",
                        "sampler2DArray",
                        "samplerCubeArray",
                        "samplerBuffer",
                        "sampler2DMS",
                        "sampler2DMSArray",
                        "isampler1D",
                        "isampler2D",
                        "isampler3D",
                        "isamplerCube",
                        "isampler2DRect",
                        "isampler1DArray",
                        "isampler2DArray",
                        "isamplerCubeArray",
                        "isamplerBuffer",
                        "isampler2DMS",
                        "isampler2DMSArray",
                        "usampler1D",
                        "usampler2D",
                        "usampler3D",
                        "usamplerCube",
                        "usampler2DRect",
                        "usampler1DArray",
                        "usampler2DArray",
                        "usamplerCubeArray",
                        "usamplerBuffer",
                        "usampler2DMS",
                        "usampler2DMSArray",
                        "sampler1DShadow",
                        "sampler2DShadow",
                        "samplerCubeShadow",
                        "sampler2DRectShadow",
                        "sampler1DArrayShadow",
                        "sampler2DArrayShadow",
                        "samplerCubeArrayShadow",
                        "image1D",
                        "image2D",
                        "image3D",
                        "imageCube",
                        "image2DRect",
                        "image1DArray",
                        "image2DArray",
                        "imageCubeArray",
                        "imageBuffer",
                        "image2DMS",
                        "image2DMSArray",
                        "iimage1D",
                        "iimage2D",
                        "iimage3D",
                        "iimageCube",
                        "iimage2DRect",
                        "iimage1DArray",
                        "iimage2DArray",
                        "iimageCubeArray",
                        "iimageBuffer",
                        "iimage2DMS",
                        "iimage2DMSArray",
                        "uimage1D",
                        "uimage2D",
                        "uimage3D",
                        "uimageCube",
                        "uimage2DRect",
                        "uimage1DArray",
                        "uimage2DArray",
                        "uimageCubeArray",
                        "uimageBuffer",
                        "uimage2DMS",
                        "uimage2DMSArray",
                    });

  addIdentifiers(lang,
                 {
                     "gl_VertexID",
                     "gl_InstanceID",
                     "gl_DrawID",
                     "gl_BaseVertex",
                     "gl_BaseInstance",
                     "gl_Position",
                     "gl_PointSize",
                     "gl_ClipDistance",
                     "gl_CullDistance",
                     "gl_FragCoord",
                     "gl_FrontFacing",
                     "gl_PointCoord",
                     "gl_FragDepth",
                     "gl_SampleID",
                     "gl_SamplePosition",
                     "gl_SampleMask",
                     "gl_Layer",
                     "gl_ViewportIndex",
                     "gl_PrimitiveID",
                     "gl_InvocationID",
                     "gl_PatchVerticesIn",
                     "gl_TessLevelOuter",
                     "gl_TessLevelInner",
                     "gl_WorkGroupID",
                     "gl_LocalInvocationID",
                     "gl_GlobalInvocationID",
                     "gl_LocalInvocationIndex",
                     "gl_NumWorkGroups",
                     "gl_WorkGroupSize",
                 },
                 "GLSL built-in variable");

  addIdentifiers(lang,
                 {
                     "radians",
                     "degrees",
                     "sin",
                     "cos",
                     "tan",
                     "asin",
                     "acos",
                     "atan",
                     "sinh",
                     "cosh",
                     "tanh",
                     "asinh",
                     "acosh",
                     "atanh",
                     "pow",
                     "exp",
                     "log",
                     "exp2",
                     "log2",
                     "sqrt",
                     "inversesqrt",
                     "abs",
                     "sign",
                     "floor",
                     "trunc",
                     "round",
                     "roundEven",
                     "ceil",
                     "fract",
                     "mod",
                     "modf",
                     "min",
                     "max",
                     "clamp",
                     "mix",
                     "step",
                     "smoothstep",
                     "isnan",
                     "isinf",
                     "floatBitsToInt",
                     "floatBitsToUint",
                     "intBitsToFloat",
                     "uintBitsToFloat",
                     "fma",
                     "frexp",
                     "ldexp",
                 },
                 "GLSL math function");

  addIdentifiers(lang,
                 {
                     "length",
                     "distance",
                     "dot",
                     "cross",
                     "normalize",
                     "faceforward",
                     "reflect",
                     "refract",
                     "matrixCompMult",
                     "outerProduct",
                     "transpose",
                     "determinant",
                     "inverse",
                 },
                 "GLSL vector/matrix function");

  addIdentifiers(lang,
                 {
                     "lessThan",
                     "lessThanEqual",
                     "greaterThan",
                     "greaterThanEqual",
                     "equal",
                     "notEqual",
                     "any",
                     "all",
                     "not",
                 },
                 "GLSL vector relational function");

  addIdentifiers(lang,
                 {
                     "textureSize",
                     "textureQueryLod",
                     "textureQueryLevels",
                     "textureSamples",
                     "texture",
                     "textureProj",
                     "textureLod",
                     "textureOffset",
                     "texelFetch",
                     "texelFetchOffset",
                     "textureProjOffset",
                     "textureLodOffset",
                     "textureProjLod",
                     "textureProjLodOffset",
                     "textureGrad",
                     "textureGradOffset",
                     "textureProjGrad",
                     "textureProjGradOffset",
                     "textureGather",
                     "textureGatherOffset",
                     "textureGatherOffsets",
                 },
                 "GLSL texture function");

  addIdentifiers(lang,
                 {
                     "dFdx",
                     "dFdy",
                     "fwidth",
                     "dFdxFine",
                     "dFdyFine",
                     "fwidthFine",
                     "dFdxCoarse",
                     "dFdyCoarse",
                     "fwidthCoarse",
                 },
                 "GLSL derivative function");

  addIdentifiers(lang,
                 {
                     "packUnorm2x16",
                     "packSnorm2x16",
                     "packUnorm4x8",
                     "packSnorm4x8",
                     "unpackUnorm2x16",
                     "unpackSnorm2x16",
                     "unpackUnorm4x8",
                     "unpackSnorm4x8",
                     "packDouble2x32",
                     "unpackDouble2x32",
                     "packHalf2x16",
                     "unpackHalf2x16",
                 },
                 "GLSL packing function");

  addIdentifiers(lang,
                 {
                     "barrier",
                     "memoryBarrier",
                     "memoryBarrierAtomicCounter",
                     "memoryBarrierBuffer",
                     "memoryBarrierShared",
                     "memoryBarrierImage",
                     "groupMemoryBarrier",
                     "subgroupBarrier",
                     "subgroupMemoryBarrier",
                     "subgroupMemoryBarrierBuffer",
                     "subgroupMemoryBarrierShared",
                     "subgroupMemoryBarrierImage",
                 },
                 "GLSL synchronization function");

  addIdentifiers(lang,
                 {
                     "atomicAdd",
                     "atomicMin",
                     "atomicMax",
                     "atomicAnd",
                     "atomicOr",
                     "atomicXor",
                     "atomicExchange",
                     "atomicCompSwap",
                     "atomicCounter",
                     "atomicCounterIncrement",
                     "atomicCounterDecrement",
                 },
                 "GLSL atomic function");

  addIdentifiers(lang,
                 {
                     "imageSize",
                     "imageSamples",
                     "imageLoad",
                     "imageStore",
                     "imageAtomicAdd",
                     "imageAtomicMin",
                     "imageAtomicMax",
                     "imageAtomicAnd",
                     "imageAtomicOr",
                     "imageAtomicXor",
                     "imageAtomicExchange",
                     "imageAtomicCompSwap",
                 },
                 "GLSL image function");

  addIdentifiers(lang,
                 {
                     "EmitStreamVertex",
                     "EndStreamPrimitive",
                     "EmitVertex",
                     "EndPrimitive",
                     "interpolateAtCentroid",
                     "interpolateAtSample",
                     "interpolateAtOffset",
                 },
                 "GLSL shader-stage function");

  addPreprocIdentifiers(lang,
                        {
                            "define",
                            "undef",
                            "if",
                            "ifdef",
                            "ifndef",
                            "else",
                            "elif",
                            "endif",
                            "error",
                            "pragma",
                            "extension",
                            "version",
                            "line",
                            "include",
                            "__LINE__",
                            "__FILE__",
                            "__VERSION__",
                            "GL_ES",
                            "GL_core_profile",
                            "GL_compatibility_profile",
                            "GL_ARB_separate_shader_objects",
                            "GL_ARB_shading_language_420pack",
                            "GL_ARB_explicit_attrib_location",
                            "GL_ARB_explicit_uniform_location",
                            "GL_ARB_enhanced_layouts",
                            "GL_EXT_scalar_block_layout",
                            "GL_EXT_nonuniform_qualifier",
                            "GL_EXT_buffer_reference",
                            "GL_EXT_shader_explicit_arithmetic_types",
                            "GL_GOOGLE_include_directive",
                            "GL_GOOGLE_cpp_style_line_directive",
                        },
                        "GLSL preprocessor identifier");

  return lang;
}

auto ShaderEditor::materialDarkPalette() -> TextEditor::Palette {
  TextEditor::Palette p;

  p[static_cast<int>(TextEditor::PaletteIndex::Default)] =
      abgr(0xEE, 0xEF, 0xF0, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::Background)] =
      abgr(0x1A, 0x1B, 0x26, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::Cursor)] =
      abgr(0xFF, 0xFF, 0xFF, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::Selection)] =
      abgr(0x82, 0xAA, 0xFF, 0x35);
  p[static_cast<int>(TextEditor::PaletteIndex::LineNumber)] =
      abgr(0x55, 0x6E, 0x7A, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::CurrentLineFill)] =
      abgr(0x82, 0xAA, 0xFF, 0x0C);
  p[static_cast<int>(TextEditor::PaletteIndex::CurrentLineFillInactive)] =
      abgr(0x82, 0xAA, 0xFF, 0x06);
  p[static_cast<int>(TextEditor::PaletteIndex::CurrentLineEdge)] =
      abgr(0x82, 0xAA, 0xFF, 0x18);

  p[static_cast<int>(TextEditor::PaletteIndex::Keyword)] =
      abgr(0xC7, 0x92, 0xEA, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::Number)] =
      abgr(0xF7, 0x8C, 0x6C, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::String)] =
      abgr(0xC3, 0xE8, 0x8D, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::CharLiteral)] =
      abgr(0xC3, 0xE8, 0x8D, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::Punctuation)] =
      abgr(0xA0, 0xA8, 0xB8, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::Preprocessor)] =
      abgr(0xFF, 0x51, 0x70, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::Identifier)] =
      abgr(0xEE, 0xEF, 0xF0, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::KnownIdentifier)] =
      abgr(0x82, 0xAA, 0xFF, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::PreprocIdentifier)] =
      abgr(0xFF, 0xCB, 0x6B, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::Comment)] =
      abgr(0x55, 0x6E, 0x7A, 0xFF);
  p[static_cast<int>(TextEditor::PaletteIndex::MultiLineComment)] =
      abgr(0x55, 0x6E, 0x7A, 0xFF);

  p[static_cast<int>(TextEditor::PaletteIndex::ErrorMarker)] =
      abgr(0xFF, 0x53, 0x70, 0x22);
  p[static_cast<int>(TextEditor::PaletteIndex::Breakpoint)] =
      abgr(0x82, 0xAA, 0xFF, 0xFF);

  return p;
}

auto ShaderEditor::makeEditor() -> TextEditor {
  TextEditor ed;
  ed.SetLanguageDefinition(makeGlslLanguageDefinition());
  ed.SetPalette(materialDarkPalette());
  ed.SetShowWhitespaces(false);
  ed.SetTabSize(4);
  return ed;
}

ShaderEditor::ShaderEditor(exec::RenderGraph &graph)
    : UiComponent("Shader Editor"), graph_(graph), vert_editor_(makeEditor()),
      frag_editor_(makeEditor()) {
  reloadFromPipeline();
}

auto ShaderEditor::collectTargets() -> std::vector<PipelineTarget> {
  std::vector<PipelineTarget> targets{};

  for (auto passRef : graph_.passes()) {
    auto &pass = passRef.get();

    auto pipeline = pass.editablePipeline();
    if (!pipeline) {
      continue;
    }

    const auto &pipelineDesc = pipeline->get().desc();
    targets.push_back(PipelineTarget{
        .passName = pass.name(),
        .pipelineName = pipelineDesc.name.empty()
                            ? std::string{"<unnamed pipeline>"}
                            : pipelineDesc.name,
        .pipeline = pipeline->get(),
    });
  }

  return targets;
}

auto ShaderEditor::activeTarget() -> std::optional<PipelineTarget> {
  auto targets = collectTargets();

  if (targets.empty()) {
    selected_pass_name_.clear();
    return std::nullopt;
  }

  for (const auto &target : targets) {
    if (target.passName == selected_pass_name_) {
      return target;
    }
  }

  selected_pass_name_ = targets.front().passName;
  return targets.front();
}

auto ShaderEditor::activePipeline()
    -> std::optional<std::reference_wrapper<pipeline::GraphicsPipeline>> {
  const auto target = activeTarget();
  if (!target) {
    return std::nullopt;
  }

  return target->pipeline;
}

auto ShaderEditor::hasPipeline() const -> bool {
  for (auto pass : graph_.passes()) {
    if (pass.get().editablePipeline()) {
      return true;
    }
  }

  return false;
}

auto ShaderEditor::activeTargetKey() -> std::string {
  const auto target = activeTarget();
  if (!target) {
    return {};
  }

  return target->label();
}

auto ShaderEditor::currentEditor() noexcept -> TextEditor & {
  return active_tab_ == 0 ? vert_editor_ : frag_editor_;
}

auto ShaderEditor::currentEditor() const noexcept -> const TextEditor & {
  return active_tab_ == 0 ? vert_editor_ : frag_editor_;
}

auto ShaderEditor::currentFileState() noexcept -> ShaderEditorFileState & {
  return active_tab_ == 0 ? vert_file_ : frag_file_;
}

auto ShaderEditor::currentFileState() const noexcept
    -> const ShaderEditorFileState & {
  return active_tab_ == 0 ? vert_file_ : frag_file_;
}

void ShaderEditor::setStatus(std::string msg, bool isError) {
  status_message_ = std::move(msg);
  status_is_error_ = isError;
}

auto ShaderEditor::findShader(pipeline::GraphicsPipelineDesc &desc,
                              VkShaderStageFlagBits stage)
    -> std::optional<
        std::reference_wrapper<pipeline::GraphicsShaderStageDesc>> {
  for (auto &shader : desc.shaders) {
    if (shader.stage == stage) {
      return shader;
    }
  }

  return std::nullopt;
}

auto ShaderEditor::findShader(const pipeline::GraphicsPipelineDesc &desc,
                              VkShaderStageFlagBits stage)
    -> std::optional<
        std::reference_wrapper<const pipeline::GraphicsShaderStageDesc>> {
  for (const auto &shader : desc.shaders) {
    if (shader.stage == stage) {
      return shader;
    }
  }

  return std::nullopt;
}

auto ShaderEditor::readTextFile(const std::string &path) -> std::string {
  if (path.empty()) {
    return {};
  }

  std::ifstream file(path);
  if (!file.is_open()) {
    return {};
  }

  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

auto ShaderEditor::writeTextFile(const std::string &path,
                                 const std::string &text) -> bool {
  if (path.empty()) {
    return false;
  }

  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (!file.is_open()) {
    return false;
  }

  file << text;
  return file.good();
}

auto ShaderEditor::fileWriteTime(const std::string &path)
    -> std::optional<std::filesystem::file_time_type> {
  if (path.empty()) {
    return std::nullopt;
  }

  std::error_code ec;
  const auto time = std::filesystem::last_write_time(path, ec);
  if (ec) {
    return std::nullopt;
  }

  return time;
}

auto ShaderEditor::shaderPath(
    std::optional<
        std::reference_wrapper<const pipeline::GraphicsShaderStageDesc>>
        shader) -> std::string {
  if (!shader) {
    return {};
  }

  const auto &module = shader->get().module;
  if (module.sourceKind != resource::ShaderModuleSourceKind::Glsl) {
    return {};
  }

  return module.glslCompile.path;
}

auto ShaderEditor::shaderSource(
    std::optional<
        std::reference_wrapper<const pipeline::GraphicsShaderStageDesc>>
        shader) -> std::string {
  if (!shader) {
    return {};
  }

  const auto &module = shader->get().module;

  if (module.sourceKind != resource::ShaderModuleSourceKind::Glsl) {
    return {};
  }

  if (!module.glslCompile.source.empty()) {
    return module.glslCompile.source;
  }

  return readTextFile(module.glslCompile.path);
}

auto ShaderEditor::shaderLabel(const pipeline::GraphicsShaderStageDesc &shader,
                               const std::string &fallback) -> std::string {
  if (shader.module.sourceKind == resource::ShaderModuleSourceKind::Glsl &&
      !shader.module.glslCompile.label.empty()) {
    return shader.module.glslCompile.label;
  }

  return fallback;
}

auto ShaderEditor::makeShaderModule(VkShaderStageFlagBits stage,
                                    std::string source, std::string label,
                                    std::string entryPoint, bool fileBacked)
    -> resource::ShaderModuleDesc {
  resource::ShaderModuleDesc desc{};

  switch (stage) {
  case VK_SHADER_STAGE_VERTEX_BIT:
    desc = fileBacked
               ? resource::ShaderModuleDesc::vertexGlslFile(label)
               : resource::ShaderModuleDesc::vertexGlslSource(source, label);
    break;
  case VK_SHADER_STAGE_FRAGMENT_BIT:
    desc = fileBacked
               ? resource::ShaderModuleDesc::fragmentGlslFile(label)
               : resource::ShaderModuleDesc::fragmentGlslSource(source, label);
    break;
  default:
    return {};
  }

  desc.setEntryPoint(entryPoint);
  return desc;
}

void ShaderEditor::reloadFromPipelineIfChanged() {
  auto pipeline = activePipeline();

  if (!pipeline) {
    if (!loaded_target_key_.empty()) {
      reloadFromPipeline();
    }

    return;
  }

  const auto targetKey = activeTargetKey();

  if (loaded_target_key_ != targetKey) {
    reloadFromPipeline();
  }
}

void ShaderEditor::reloadFromPipeline() {
  auto pipeline = activePipeline();

  if (!pipeline) {
    selected_pass_name_.clear();
    loaded_target_key_.clear();
    const std::string message = "// No graphics pipeline available.\n";
    vert_editor_.SetText(message);
    frag_editor_.SetText(message);
    vert_file_ =
        ShaderEditorFileState{.synced_text = message, .valid_text = message};
    frag_file_ =
        ShaderEditorFileState{.synced_text = message, .valid_text = message};
    setStatus("No graphics pipeline available.", true);
    return;
  }

  const auto &desc = pipeline->get().desc();
  loaded_target_key_ = activeTargetKey();

  const auto vert = findShader(desc, VK_SHADER_STAGE_VERTEX_BIT);
  const auto frag = findShader(desc, VK_SHADER_STAGE_FRAGMENT_BIT);

  const std::string vertSource = shaderSource(vert);
  const std::string fragSource = shaderSource(frag);
  const std::string vertPath = shaderPath(vert);
  const std::string fragPath = shaderPath(frag);
  const auto vertWriteTime = fileWriteTime(vertPath);
  const auto fragWriteTime = fileWriteTime(fragPath);

  if (!vert) {
    vert_editor_.SetText("// Selected pipeline has no vertex shader stage.\n");
  } else if (vertSource.empty()) {
    vert_editor_.SetText("// Vertex shader is not GLSL source-backed.\n");
  } else {
    vert_editor_.SetText(vertSource);
  }

  if (!frag) {
    frag_editor_.SetText(
        "// Selected pipeline has no fragment shader stage.\n");
  } else if (fragSource.empty()) {
    frag_editor_.SetText("// Fragment shader is not GLSL source-backed.\n");
  } else {
    frag_editor_.SetText(fragSource);
  }

  vert_file_ = ShaderEditorFileState{
      .path = vertPath,
      .write_time = vertWriteTime.value_or(std::filesystem::file_time_type{}),
      .disk_text = vertSource,
      .synced_text = vertSource,
      .valid_text = vertSource,
      .file_backed = !vertPath.empty(),
      .conflict = false,
  };

  frag_file_ = ShaderEditorFileState{
      .path = fragPath,
      .write_time = fragWriteTime.value_or(std::filesystem::file_time_type{}),
      .disk_text = fragSource,
      .synced_text = fragSource,
      .valid_text = fragSource,
      .file_backed = !fragPath.empty(),
      .conflict = false,
  };

  vert_file_.synced_text = vert_editor_.GetText();
  frag_file_.synced_text = frag_editor_.GetText();
  vert_file_.valid_text = vert_file_.synced_text;
  frag_file_.valid_text = frag_file_.synced_text;

  if (const auto cached = pipeline_files_.find(loaded_target_key_);
      cached != pipeline_files_.end()) {
    if (!vert_file_.file_backed && cached->second.vert.file_backed) {
      vert_file_ = cached->second.vert;
      vert_editor_.SetText(vert_file_.synced_text);
    }

    if (!frag_file_.file_backed && cached->second.frag.file_backed) {
      frag_file_ = cached->second.frag;
      frag_editor_.SetText(frag_file_.synced_text);
    }
  }

  pipeline_files_[loaded_target_key_] = ShaderEditorPipelineState{
      .vert = vert_file_,
      .frag = frag_file_,
  };

  setStatus("Loaded target: " + loaded_target_key_, false);
}

void ShaderEditor::refreshFileBackedShaders(
    const std::vector<PipelineTarget> &targets) {
  const double now = ImGui::GetTime();
  const bool probeContent = now - last_file_probe_time_ >= 0.25;
  if (probeContent) {
    last_file_probe_time_ = now;
  }

  struct FileChange {
    std::filesystem::file_time_type writeTime{};
    std::string diskText{};
  };

  auto initFileState =
      [](ShaderEditorFileState &state,
         std::optional<
             std::reference_wrapper<const pipeline::GraphicsShaderStageDesc>>
             shader) -> void {
    const std::string path = shaderPath(shader);
    if (path.empty()) {
      return;
    }

    if (state.file_backed && state.path == path) {
      return;
    }

    const std::string diskText = readTextFile(path);
    state = ShaderEditorFileState{
        .path = path,
        .write_time =
            fileWriteTime(path).value_or(std::filesystem::file_time_type{}),
        .disk_text = diskText,
        .synced_text = diskText,
        .valid_text = diskText,
        .file_backed = true,
        .conflict = false,
    };
  };

  auto changedFile =
      [probeContent](
          const ShaderEditorFileState &state) -> std::optional<FileChange> {
    if (!state.file_backed) {
      return std::nullopt;
    }

    const auto time = fileWriteTime(state.path);
    const bool timeChanged = time && *time != state.write_time;
    if (!timeChanged && !probeContent) {
      return std::nullopt;
    }

    const std::string diskText = readTextFile(state.path);
    if (diskText == state.disk_text) {
      return std::nullopt;
    }

    return FileChange{.writeTime = time.value_or(state.write_time),
                      .diskText = diskText};
  };

  auto applyFileBackedState = [&](const PipelineTarget &target,
                                  ShaderEditorPipelineState &state,
                                  bool active) -> bool {
    auto &pipeline = target.pipeline.get();
    const auto oldDesc = pipeline.desc();
    auto nextDesc = oldDesc;

    if (auto vert = findShader(nextDesc, VK_SHADER_STAGE_VERTEX_BIT)) {
      auto &shader = vert->get();
      if (state.vert.file_backed) {
        shader.module =
            makeShaderModule(VK_SHADER_STAGE_VERTEX_BIT, state.vert.synced_text,
                             state.vert.path, shader.entryPoint, true);
      }
    }

    if (auto frag = findShader(nextDesc, VK_SHADER_STAGE_FRAGMENT_BIT)) {
      auto &shader = frag->get();
      if (state.frag.file_backed) {
        shader.module = makeShaderModule(
            VK_SHADER_STAGE_FRAGMENT_BIT, state.frag.synced_text,
            state.frag.path, shader.entryPoint, true);
      }
    }

    try {
      if (pipeline.update(nextDesc)) {
        state.vert.valid_text = state.vert.synced_text;
        state.frag.valid_text = state.frag.synced_text;
        state.vert.conflict = false;
        state.frag.conflict = false;

        if (active) {
          vert_file_ = state.vert;
          frag_file_ = state.frag;
        }

        setStatus("Hot reloaded: " + target.label(), false);
        return true;
      }
    } catch (const std::exception &e) {
      setStatus("Failed to hot reload: " + target.label() + ": " + e.what(),
                true);
    } catch (...) {
      setStatus("Failed to hot reload: " + target.label(), true);
    }

    auto restoreDesc = oldDesc;

    if (auto vert = findShader(restoreDesc, VK_SHADER_STAGE_VERTEX_BIT)) {
      auto &shader = vert->get();
      if (state.vert.file_backed && !state.vert.valid_text.empty()) {
        shader.module =
            makeShaderModule(VK_SHADER_STAGE_VERTEX_BIT, state.vert.valid_text,
                             shaderLabel(shader, restoreDesc.name + ".vert"),
                             shader.entryPoint, false);
      }
    }

    if (auto frag = findShader(restoreDesc, VK_SHADER_STAGE_FRAGMENT_BIT)) {
      auto &shader = frag->get();
      if (state.frag.file_backed && !state.frag.valid_text.empty()) {
        shader.module = makeShaderModule(
            VK_SHADER_STAGE_FRAGMENT_BIT, state.frag.valid_text,
            shaderLabel(shader, restoreDesc.name + ".frag"), shader.entryPoint,
            false);
      }
    }

    try {
      pipeline.update(restoreDesc);
    } catch (...) {
    }

    if (active) {
      vert_file_ = state.vert;
      frag_file_ = state.frag;
    }

    return false;
  };

  for (const auto &target : targets) {
    const std::string key = target.label();
    const bool active = key == loaded_target_key_;
    auto &state = pipeline_files_[key];

    if (active) {
      state.vert = vert_file_;
      state.frag = frag_file_;
    } else {
      const auto &desc = target.pipeline.get().desc();
      initFileState(state.vert, findShader(desc, VK_SHADER_STAGE_VERTEX_BIT));
      initFileState(state.frag, findShader(desc, VK_SHADER_STAGE_FRAGMENT_BIT));
    }

    const auto vertChange = changedFile(state.vert);
    const auto fragChange = changedFile(state.frag);
    const bool vertChanged = vertChange.has_value();
    const bool fragChanged = fragChange.has_value();
    if (!vertChanged && !fragChanged) {
      continue;
    }

    const bool editorDirty =
        active && (vert_editor_.GetText() != state.vert.synced_text ||
                   frag_editor_.GetText() != state.frag.synced_text);

    if (editorDirty) {
      state.vert.conflict = state.vert.conflict || vertChanged;
      state.frag.conflict = state.frag.conflict || fragChanged;
      vert_file_ = state.vert;
      frag_file_ = state.frag;
      setStatus("Shader file changed on disk; save to resolve.", true);
      continue;
    }

    if (vertChanged) {
      state.vert.write_time = vertChange->writeTime;
      state.vert.disk_text = vertChange->diskText;

      if (active) {
        vert_editor_.SetText(vertChange->diskText);
        state.vert.synced_text = vert_editor_.GetText();
      } else {
        state.vert.synced_text = vertChange->diskText;
      }

      state.vert.conflict = false;
    }

    if (fragChanged) {
      state.frag.write_time = fragChange->writeTime;
      state.frag.disk_text = fragChange->diskText;

      if (active) {
        frag_editor_.SetText(fragChange->diskText);
        state.frag.synced_text = frag_editor_.GetText();
      } else {
        state.frag.synced_text = fragChange->diskText;
      }

      state.frag.conflict = false;
    }

    (void)applyFileBackedState(target, state, active);
  }
}

auto ShaderEditor::applyToPipeline(bool saveFiles) -> bool {
  auto pipeline = activePipeline();

  if (!pipeline) {
    setStatus("No graphics pipeline available.", true);
    return false;
  }

  const auto oldDesc = pipeline->get().desc();

  const std::string oldVertSource =
      shaderSource(findShader(oldDesc, VK_SHADER_STAGE_VERTEX_BIT));
  const std::string oldFragSource =
      shaderSource(findShader(oldDesc, VK_SHADER_STAGE_FRAGMENT_BIT));

  const std::string nextVertSource = vert_editor_.GetText();
  const std::string nextFragSource = frag_editor_.GetText();

  auto nextDesc = oldDesc;

  auto vert = findShader(nextDesc, VK_SHADER_STAGE_VERTEX_BIT);
  auto frag = findShader(nextDesc, VK_SHADER_STAGE_FRAGMENT_BIT);

  if (!vert) {
    setStatus("Selected pipeline has no vertex shader stage.", true);
    return false;
  }

  if (!frag) {
    setStatus("Selected pipeline has no fragment shader stage.", true);
    return false;
  }

  auto &vertShader = vert->get();
  auto &fragShader = frag->get();
  const bool vertFileBacked =
      vert_file_.file_backed && !vert_file_.path.empty();
  const bool fragFileBacked =
      frag_file_.file_backed && !frag_file_.path.empty();
  const bool saveVertFile = saveFiles && vertFileBacked;
  const bool saveFragFile = saveFiles && fragFileBacked;

  if (saveVertFile && !writeTextFile(vert_file_.path, nextVertSource)) {
    setStatus("Failed to write vertex shader: " + vert_file_.path, true);
    return false;
  }

  if (saveFragFile && !writeTextFile(frag_file_.path, nextFragSource)) {
    if (saveVertFile) {
      (void)writeTextFile(vert_file_.path, oldVertSource);
      vert_file_.disk_text = oldVertSource;
      if (const auto time = fileWriteTime(vert_file_.path)) {
        vert_file_.write_time = *time;
      }
    }

    setStatus("Failed to write fragment shader: " + frag_file_.path, true);
    return false;
  }

  vertShader.module = makeShaderModule(
      VK_SHADER_STAGE_VERTEX_BIT, nextVertSource,
      vertFileBacked ? vert_file_.path
                     : shaderLabel(vertShader, nextDesc.name + ".vert"),
      vertShader.entryPoint, vertFileBacked);

  fragShader.module = makeShaderModule(
      VK_SHADER_STAGE_FRAGMENT_BIT, nextFragSource,
      fragFileBacked ? frag_file_.path
                     : shaderLabel(fragShader, nextDesc.name + ".frag"),
      fragShader.entryPoint, fragFileBacked);

  try {
    if (pipeline->get().update(nextDesc)) {
      loaded_target_key_ = activeTargetKey();
      vert_file_.synced_text = nextVertSource;
      frag_file_.synced_text = nextFragSource;
      vert_file_.valid_text = nextVertSource;
      frag_file_.valid_text = nextFragSource;
      if (saveVertFile) {
        vert_file_.disk_text = nextVertSource;
      }

      if (saveFragFile) {
        frag_file_.disk_text = nextFragSource;
      }

      vert_file_.conflict = false;
      frag_file_.conflict = false;

      if (const auto time = fileWriteTime(vert_file_.path)) {
        vert_file_.write_time = *time;
      }

      if (const auto time = fileWriteTime(frag_file_.path)) {
        frag_file_.write_time = *time;
      }

      if (!loaded_target_key_.empty()) {
        pipeline_files_[loaded_target_key_] = ShaderEditorPipelineState{
            .vert = vert_file_,
            .frag = frag_file_,
        };
      }

      setStatus((saveVertFile || saveFragFile ? "Saved and applied: "
                                              : "Compiled and applied: ") +
                    loaded_target_key_,
                false);
      return true;
    }
  } catch (const std::exception &e) {
    setStatus("Failed to compile pipeline. Restored previous pipeline: " +
                  std::string(e.what()),
              true);
  } catch (...) {
    setStatus("Failed to compile pipeline. Restored previous pipeline.", true);
  }

  auto restoreDesc = oldDesc;

  if (auto restoreVert = findShader(restoreDesc, VK_SHADER_STAGE_VERTEX_BIT)) {
    auto &shader = restoreVert->get();
    const std::string source =
        vert_file_.valid_text.empty() ? oldVertSource : vert_file_.valid_text;
    if (!source.empty()) {
      shader.module =
          makeShaderModule(VK_SHADER_STAGE_VERTEX_BIT, source,
                           shaderLabel(shader, restoreDesc.name + ".vert"),
                           shader.entryPoint, false);
    }
  }

  if (auto restoreFrag =
          findShader(restoreDesc, VK_SHADER_STAGE_FRAGMENT_BIT)) {
    auto &shader = restoreFrag->get();
    const std::string source =
        frag_file_.valid_text.empty() ? oldFragSource : frag_file_.valid_text;
    if (!source.empty()) {
      shader.module =
          makeShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, source,
                           shaderLabel(shader, restoreDesc.name + ".frag"),
                           shader.entryPoint, false);
    }
  }

  try {
    pipeline->get().update(restoreDesc);
  } catch (...) {
  }

  if (saveVertFile) {
    (void)writeTextFile(vert_file_.path, oldVertSource);
    vert_file_.disk_text = oldVertSource;
    if (const auto time = fileWriteTime(vert_file_.path)) {
      vert_file_.write_time = *time;
    }
  }

  if (saveFragFile) {
    (void)writeTextFile(frag_file_.path, oldFragSource);
    frag_file_.disk_text = oldFragSource;
    if (const auto time = fileWriteTime(frag_file_.path)) {
      frag_file_.write_time = *time;
    }
  }

  loaded_target_key_ = activeTargetKey();

  if (status_message_.empty() || !status_is_error_) {
    setStatus("Failed to compile pipeline. Previous pipeline was restored.",
              true);
  }

  if (!loaded_target_key_.empty()) {
    pipeline_files_[loaded_target_key_] = ShaderEditorPipelineState{
        .vert = vert_file_,
        .frag = frag_file_,
    };
  }

  return false;
}

void ShaderEditor::renderTargetSelector(
    const std::vector<PipelineTarget> &targets) {
  const bool hasTargets = !targets.empty();
  const auto active = activeTarget();
  const std::string selectedLabel =
      active ? active->label() : std::string{"No editable passes"};

  if (!hasTargets) {
    ImGui::BeginDisabled();
  }

  ImGui::SetNextItemWidth(-1.0f);
  if (ImGui::BeginCombo("##shader_target", selectedLabel.c_str())) {
    for (const auto &target : targets) {
      const bool selected = target.passName == selected_pass_name_;

      if (ImGui::Selectable(target.label().c_str(), selected)) {
        selected_pass_name_ = target.passName;
        reloadFromPipeline();
      }

      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }

    ImGui::EndCombo();
  }

  if (!hasTargets) {
    ImGui::EndDisabled();
  }
}

auto ShaderEditor::parseErrors(const std::string &log)
    -> TextEditor::ErrorMarkers {
  TextEditor::ErrorMarkers markers;
  std::istringstream ss(log);
  std::string line;

  while (std::getline(ss, line)) {
    int lineNo = 0;

    if (std::sscanf(line.c_str(), "ERROR: %*d:%d:", &lineNo) == 1 &&
        lineNo > 0) {
      markers[lineNo] += (markers.count(lineNo) ? "\n" : "") + line;
    } else if (std::sscanf(line.c_str(), "%*[^:]:%d:", &lineNo) == 1 &&
               lineNo > 0) {
      markers[lineNo] += (markers.count(lineNo) ? "\n" : "") + line;
    }
  }

  return markers;
}

auto ShaderEditor::render() -> void {
  reloadFromPipelineIfChanged();
  const auto targets = collectTargets();
  refreshFileBackedShaders(targets);

  const ImGuiStyle &style = ImGui::GetStyle();
  const ImVec4 bg = style.Colors[ImGuiCol_WindowBg];
  const ImVec4 childBg = style.Colors[ImGuiCol_ChildBg];
  const ImVec4 text = style.Colors[ImGuiCol_Text];
  const ImVec4 textDisabled = style.Colors[ImGuiCol_TextDisabled];
  const ImVec4 accent = style.Colors[ImGuiCol_CheckMark];
  const ImVec4 selection = style.Colors[ImGuiCol_TextSelectedBg];
  const ImVec4 frameBg = style.Colors[ImGuiCol_FrameBg];
  const ImVec4 editorBg = childBg.w > 0.0f ? childBg : bg;
  const bool lightBackground = relativeLuminance(editorBg) > 0.55f;
  const ImVec4 editorAccent =
      lightBackground
          ? ImVec4(accent.x * 0.62f, accent.y * 0.62f, accent.z * 0.62f, 1.0f)
          : accent;

  auto syncPalette = [&](TextEditor &ed) -> void {
    auto palette = ed.GetPalette();

    palette[static_cast<int>(TextEditor::PaletteIndex::Default)] =
        styleColorToAbgr(text);
    palette[static_cast<int>(TextEditor::PaletteIndex::Background)] =
        styleColorToAbgr(editorBg);
    palette[static_cast<int>(TextEditor::PaletteIndex::Cursor)] =
        styleColorToAbgr(text);
    palette[static_cast<int>(TextEditor::PaletteIndex::Selection)] =
        styleColorToAbgr(selection);
    palette[static_cast<int>(TextEditor::PaletteIndex::LineNumber)] =
        styleColorToAbgr(textDisabled);
    palette[static_cast<int>(TextEditor::PaletteIndex::CurrentLineFill)] =
        styleColorToAbgr(frameBg, 0.35f);
    palette[static_cast<int>(
        TextEditor::PaletteIndex::CurrentLineFillInactive)] =
        styleColorToAbgr(frameBg, 0.18f);
    palette[static_cast<int>(TextEditor::PaletteIndex::CurrentLineEdge)] =
        styleColorToAbgr(editorAccent, 0.35f);
    palette[static_cast<int>(TextEditor::PaletteIndex::Keyword)] =
        lightBackground ? abgr(0x64, 0x28, 0x9E, 0xFF)
                        : abgr(0xC7, 0x92, 0xEA, 0xFF);
    palette[static_cast<int>(TextEditor::PaletteIndex::Number)] =
        lightBackground ? abgr(0x8A, 0x3F, 0x00, 0xFF)
                        : abgr(0xF7, 0x8C, 0x6C, 0xFF);
    palette[static_cast<int>(TextEditor::PaletteIndex::String)] =
        lightBackground ? abgr(0x2E, 0x6B, 0x2E, 0xFF)
                        : abgr(0xC3, 0xE8, 0x8D, 0xFF);
    palette[static_cast<int>(TextEditor::PaletteIndex::CharLiteral)] =
        lightBackground ? abgr(0x2E, 0x6B, 0x2E, 0xFF)
                        : abgr(0xC3, 0xE8, 0x8D, 0xFF);
    palette[static_cast<int>(TextEditor::PaletteIndex::Punctuation)] =
        lightBackground ? abgr(0x49, 0x54, 0x63, 0xFF)
                        : abgr(0xA0, 0xA8, 0xB8, 0xFF);
    palette[static_cast<int>(TextEditor::PaletteIndex::Preprocessor)] =
        lightBackground ? abgr(0xB4, 0x23, 0x4F, 0xFF)
                        : abgr(0xFF, 0x51, 0x70, 0xFF);
    palette[static_cast<int>(TextEditor::PaletteIndex::Identifier)] =
        styleColorToAbgr(text);
    palette[static_cast<int>(TextEditor::PaletteIndex::KnownIdentifier)] =
        styleColorToAbgr(editorAccent);
    palette[static_cast<int>(TextEditor::PaletteIndex::PreprocIdentifier)] =
        lightBackground ? abgr(0x8A, 0x52, 0x00, 0xFF)
                        : abgr(0xFF, 0xCB, 0x6B, 0xFF);
    palette[static_cast<int>(TextEditor::PaletteIndex::Comment)] =
        lightBackground ? abgr(0x5A, 0x64, 0x72, 0xFF)
                        : abgr(0x55, 0x6E, 0x7A, 0xFF);
    palette[static_cast<int>(TextEditor::PaletteIndex::MultiLineComment)] =
        lightBackground ? abgr(0x5A, 0x64, 0x72, 0xFF)
                        : abgr(0x55, 0x6E, 0x7A, 0xFF);
    palette[static_cast<int>(TextEditor::PaletteIndex::ErrorMarker)] =
        lightBackground ? abgr(0xD9, 0x2D, 0x20, 0x22)
                        : abgr(0xFF, 0x53, 0x70, 0x22);
    palette[static_cast<int>(TextEditor::PaletteIndex::Breakpoint)] =
        styleColorToAbgr(editorAccent);

    ed.SetPalette(palette);
  };

  syncPalette(vert_editor_);
  syncPalette(frag_editor_);

  ImGui::TextUnformatted("Shader Target");
  renderTargetSelector(targets);

  const float spacingY = style.ItemSpacing.y;
  const float frameH = ImGui::GetFrameHeight();
  const float textLineH = ImGui::GetTextLineHeight();

  const float statusH = status_message_.empty() ? 0.0f : textLineH + spacingY;
  const float headerH = textLineH + frameH + spacingY * 2.0f;
  const float tabsH = frameH + spacingY;
  const float bottomBarH = spacingY + 1.0f + spacingY + statusH + frameH +
                           spacingY + textLineH + spacingY;
  const float codeH =
      ImGui::GetContentRegionAvail().y - headerH - tabsH - bottomBarH;

  if (ImGui::BeginTabBar("ShaderTabs")) {
    if (ImGui::BeginTabItem("  Vertex  ")) {
      active_tab_ = 0;
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("  Fragment  ")) {
      active_tab_ = 1;
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  TextEditor &editor = currentEditor();
  ShaderEditorFileState &fileState = currentFileState();

  if (status_is_error_ && !status_message_.empty()) {
    editor.SetErrorMarkers(parseErrors(status_message_));
  } else {
    editor.SetErrorMarkers({});
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));

  if (ImGui::BeginChild("##shader_code_area", ImVec2(-1.0f, codeH), true,
                        ImGuiWindowFlags_NoScrollbar)) {
    if (ImGui::GetIO().Fonts->Fonts.Size > 1) {
      ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    }

    editor.Render("##shader_source", ImVec2(-1.0f, -1.0f));

    if (ImGui::GetIO().Fonts->Fonts.Size > 1) {
      ImGui::PopFont();
    }
  }

  ImGui::EndChild();
  ImGui::PopStyleVar();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (!status_message_.empty()) {
    const ImVec4 statusColor = status_is_error_
                                   ? ImVec4(1.0f, 0.35f, 0.35f, 1.0f)
                                   : ImVec4(0.55f, 1.0f, 0.55f, 1.0f);

    ImGui::TextColored(statusColor, "%s", status_message_.c_str());
    ImGui::Spacing();
  }

  const bool canEdit = hasPipeline();

  if (!canEdit) {
    ImGui::BeginDisabled();
  }

  const bool fileBacked = fileState.file_backed;
  if (ImGui::Button(fileBacked ? "Save & Apply" : "Compile & Apply",
                    ImVec2(150.0f, 0.0f))) {
    setStatus(fileBacked ? "Saving..." : "Compiling...", false);
    (void)applyToPipeline();
  }

  if (!canEdit) {
    ImGui::EndDisabled();
  }

  ImGui::SameLine(0.0f, 8.0f);

  if (ImGui::Button("Undo", ImVec2(54.0f, 0.0f))) {
    editor.Undo();
  }

  ImGui::SameLine(0.0f, 4.0f);

  if (ImGui::Button("Redo", ImVec2(54.0f, 0.0f))) {
    editor.Redo();
  }

  auto coords = editor.GetCursorPosition();
  const bool dirty = editor.GetText() != fileState.synced_text;
  std::array<char, 80> info{};

  std::snprintf(info.data(), info.size(), "%s%sLn %d   Col %d",
                dirty ? "modified   " : "",
                fileState.conflict ? "conflict   " : "", coords.mLine + 1,
                coords.mColumn + 1);

  const float infoW = ImGui::CalcTextSize(info.data()).x;
  const float cursorX = ImGui::GetCursorPosX();
  const float rightX = cursorX + ImGui::GetContentRegionAvail().x - infoW;

  ImGui::SetCursorPosX(rightX > cursorX ? rightX : cursorX);
  ImGui::TextDisabled("%s", info.data());
}

} // namespace vkr::ui
