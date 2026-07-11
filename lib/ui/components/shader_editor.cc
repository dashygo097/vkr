#include "vkr/ui/components/shader_editor.hh"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
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

ShaderEditor::ShaderEditor(render::PipelineLibrary &pipelineLibrary)
    : pipeline_library_(pipelineLibrary), vert_editor_(makeEditor()),
      frag_editor_(makeEditor()) {
  reloadFromPipeline();
}

auto ShaderEditor::activePipeline() noexcept -> pipeline::GraphicsPipeline * {
  if (pipeline_library_.empty()) {
    return nullptr;
  }

  return &pipeline_library_.first();
}

auto ShaderEditor::hasPipeline() const noexcept -> bool {
  return !pipeline_library_.empty();
}

auto ShaderEditor::currentEditor() noexcept -> TextEditor & {
  return active_tab_ == 0 ? vert_editor_ : frag_editor_;
}

auto ShaderEditor::currentEditor() const noexcept -> const TextEditor & {
  return active_tab_ == 0 ? vert_editor_ : frag_editor_;
}

void ShaderEditor::setStatus(std::string msg, bool isError) {
  status_message_ = std::move(msg);
  status_is_error_ = isError;
}

auto ShaderEditor::findShader(pipeline::GraphicsPipelineDesc &desc,
                              VkShaderStageFlagBits stage)
    -> pipeline::GraphicsShaderStageDesc * {
  for (auto &shader : desc.shaders) {
    if (shader.stage == stage) {
      return &shader;
    }
  }

  return nullptr;
}

auto ShaderEditor::findShader(const pipeline::GraphicsPipelineDesc &desc,
                              VkShaderStageFlagBits stage)
    -> const pipeline::GraphicsShaderStageDesc * {
  for (const auto &shader : desc.shaders) {
    if (shader.stage == stage) {
      return &shader;
    }
  }

  return nullptr;
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

auto ShaderEditor::shaderSource(const pipeline::GraphicsShaderStageDesc *shader)
    -> std::string {
  if (shader == nullptr) {
    return {};
  }

  const auto &module = shader->module;

  if (module.sourceKind != resource::ShaderModuleSourceKind::Glsl) {
    return {};
  }

  if (!module.compile.source.empty()) {
    return module.compile.source;
  }

  return readTextFile(module.compile.label);
}

auto ShaderEditor::shaderLabel(const pipeline::GraphicsShaderStageDesc &shader,
                               const std::string &fallback) -> std::string {
  if (shader.module.sourceKind == resource::ShaderModuleSourceKind::Glsl &&
      !shader.module.compile.label.empty()) {
    return shader.module.compile.label;
  }

  return fallback;
}

auto ShaderEditor::makeShaderModule(VkShaderStageFlagBits stage,
                                    std::string source, std::string label,
                                    std::string entryPoint)
    -> resource::ShaderModuleDesc {
  resource::ShaderModuleDesc desc{};

  switch (stage) {
  case VK_SHADER_STAGE_VERTEX_BIT:
    desc = resource::ShaderModuleDesc::vertexGlslSource(source, label);
    break;
  case VK_SHADER_STAGE_FRAGMENT_BIT:
    desc = resource::ShaderModuleDesc::fragmentGlslSource(source, label);
    break;
  default:
    return {};
  }

  desc.setEntryPoint(entryPoint);
  return desc;
}

void ShaderEditor::reloadFromPipelineIfChanged() {
  auto *pipeline = activePipeline();

  if (pipeline == nullptr) {
    if (!loaded_pipeline_name_.empty()) {
      reloadFromPipeline();
    }

    return;
  }

  const auto &desc = pipeline->desc();

  if (loaded_pipeline_name_ != desc.name) {
    reloadFromPipeline();
  }
}

void ShaderEditor::reloadFromPipeline() {
  auto *pipeline = activePipeline();

  if (pipeline == nullptr) {
    loaded_pipeline_name_.clear();
    vert_editor_.SetText("// No graphics pipeline available.\n");
    frag_editor_.SetText("// No graphics pipeline available.\n");
    setStatus("No graphics pipeline available.", true);
    return;
  }

  const auto &desc = pipeline->desc();
  loaded_pipeline_name_ = desc.name;

  const auto *vert = findShader(desc, VK_SHADER_STAGE_VERTEX_BIT);
  const auto *frag = findShader(desc, VK_SHADER_STAGE_FRAGMENT_BIT);

  const std::string vertSource = shaderSource(vert);
  const std::string fragSource = shaderSource(frag);

  if (vert == nullptr) {
    vert_editor_.SetText("// Selected pipeline has no vertex shader stage.\n");
  } else if (vertSource.empty()) {
    vert_editor_.SetText("// Vertex shader is not GLSL source-backed.\n");
  } else {
    vert_editor_.SetText(vertSource);
  }

  if (frag == nullptr) {
    frag_editor_.SetText(
        "// Selected pipeline has no fragment shader stage.\n");
  } else if (fragSource.empty()) {
    frag_editor_.SetText("// Fragment shader is not GLSL source-backed.\n");
  } else {
    frag_editor_.SetText(fragSource);
  }

  setStatus("Loaded pipeline: " + loaded_pipeline_name_, false);
}

void ShaderEditor::applyToPipeline() {
  auto *pipeline = activePipeline();

  if (pipeline == nullptr) {
    setStatus("No graphics pipeline available.", true);
    return;
  }

  auto nextDesc = pipeline->desc();

  auto *vert = findShader(nextDesc, VK_SHADER_STAGE_VERTEX_BIT);
  auto *frag = findShader(nextDesc, VK_SHADER_STAGE_FRAGMENT_BIT);

  if (vert == nullptr) {
    setStatus("Selected pipeline has no vertex shader stage.", true);
    return;
  }

  if (frag == nullptr) {
    setStatus("Selected pipeline has no fragment shader stage.", true);
    return;
  }

  vert->module = makeShaderModule(
      VK_SHADER_STAGE_VERTEX_BIT, vert_editor_.GetText(),
      shaderLabel(*vert, nextDesc.name + ".vert"), vert->entryPoint);

  frag->module = makeShaderModule(
      VK_SHADER_STAGE_FRAGMENT_BIT, frag_editor_.GetText(),
      shaderLabel(*frag, nextDesc.name + ".frag"), frag->entryPoint);

  if (!pipeline->update(nextDesc)) {
    setStatus("Failed to compile pipeline. Previous pipeline was kept.", true);
    return;
  }

  loaded_pipeline_name_ = nextDesc.name;
  setStatus("Compiled and applied: " + loaded_pipeline_name_, false);
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

  const ImGuiStyle &style = ImGui::GetStyle();
  const ImVec4 bg = style.Colors[ImGuiCol_WindowBg];
  const ImVec4 childBg = style.Colors[ImGuiCol_ChildBg];
  const ImVec4 text = style.Colors[ImGuiCol_Text];
  const ImVec4 textDisabled = style.Colors[ImGuiCol_TextDisabled];
  const ImVec4 accent = style.Colors[ImGuiCol_CheckMark];
  const ImVec4 selection = style.Colors[ImGuiCol_TextSelectedBg];
  const ImVec4 frameBg = style.Colors[ImGuiCol_FrameBg];

  auto syncPalette = [&](TextEditor &ed) -> void {
    auto palette = ed.GetPalette();

    palette[static_cast<int>(TextEditor::PaletteIndex::Default)] =
        styleColorToAbgr(text);
    palette[static_cast<int>(TextEditor::PaletteIndex::Background)] =
        styleColorToAbgr(childBg.w > 0.0f ? childBg : bg);
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
        styleColorToAbgr(accent, 0.35f);
    palette[static_cast<int>(TextEditor::PaletteIndex::KnownIdentifier)] =
        styleColorToAbgr(accent);
    palette[static_cast<int>(TextEditor::PaletteIndex::Breakpoint)] =
        styleColorToAbgr(accent);

    ed.SetPalette(palette);
  };

  syncPalette(vert_editor_);
  syncPalette(frag_editor_);

  ImGui::TextDisabled("Pipeline: %s", loaded_pipeline_name_.empty()
                                          ? "<none>"
                                          : loaded_pipeline_name_.c_str());

  const float spacingY = style.ItemSpacing.y;
  const float frameH = ImGui::GetFrameHeight();
  const float textLineH = ImGui::GetTextLineHeight();

  const float statusH = status_message_.empty() ? 0.0f : textLineH + spacingY;
  const float headerH = textLineH + spacingY;
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

  if (status_is_error_ && !status_message_.empty()) {
    editor.SetErrorMarkers(parseErrors(status_message_));
  } else {
    editor.SetErrorMarkers({});
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));

  if (ImGui::BeginChild("##shader_code_area", ImVec2(-1.0f, codeH), false,
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

  if (ImGui::Button("Compile & Apply", ImVec2(150.0f, 0.0f))) {
    setStatus("Compiling...", false);
    applyToPipeline();
  }

  ImGui::SameLine(0.0f, 8.0f);

  if (ImGui::Button("Reload", ImVec2(70.0f, 0.0f))) {
    reloadFromPipeline();
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
  std::array<char, 40> info{};

  std::snprintf(info.data(), info.size(), "Ln %d   Col %d", coords.mLine + 1,
                coords.mColumn + 1);

  const float infoW = ImGui::CalcTextSize(info.data()).x;
  const float cursorX = ImGui::GetCursorPosX();
  const float rightX = cursorX + ImGui::GetContentRegionAvail().x - infoW;

  ImGui::SetCursorPosX(rightX > cursorX ? rightX : cursorX);
  ImGui::TextDisabled("%s", info.data());
}

} // namespace vkr::ui
