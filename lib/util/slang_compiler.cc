#include "vkr/logger.hh"
#include "vkr/util/compiler.hh"
#include "vkr/util/io.hh"

#ifdef VKR_HAS_SLANG
#include <slang/slang-com-ptr.h>
#include <slang/slang.h>
#endif

#include <cstring>
#include <string>
#include <vector>

namespace vkr::util {

namespace {

auto loadSlangSource(const SlangCompileDesc &desc) -> std::string {
  if (!desc.source.empty()) {
    return desc.source;
  }

  return fread_string(desc.path);
}

#ifdef VKR_HAS_SLANG

auto blobString(ISlangBlob *blob) -> std::string {
  if (blob == nullptr || blob->getBufferPointer() == nullptr ||
      blob->getBufferSize() == 0) {
    return {};
  }

  return {static_cast<const char *>(blob->getBufferPointer()),
          blob->getBufferSize()};
}

auto toSlangStage(SlangShaderStage stage) -> SlangStage {
  switch (stage) {
  case SlangShaderStage::Vertex:
    return SLANG_STAGE_VERTEX;
  case SlangShaderStage::Fragment:
    return SLANG_STAGE_FRAGMENT;
  case SlangShaderStage::Compute:
    return SLANG_STAGE_COMPUTE;
  }

  return SLANG_STAGE_NONE;
}

#endif

} // namespace

auto ShaderCompiler::compileSlang(const SlangCompileDesc &desc)
    -> ShaderCompileResult {
  ShaderCompileResult result{};

  if (!desc.isValid()) {
    result.error = "invalid Slang compile descriptor";
    return result;
  }

  const std::string source = loadSlangSource(desc);
  if (source.empty()) {
    result.error = "Slang shader source is empty: " + desc.label;
    return result;
  }

#ifndef VKR_HAS_SLANG
  result.error = "VKR was built without Slang support";
  return result;
#else
  VKR_UTIL_INFO("Compiling Slang shader: {}", desc.label);

  Slang::ComPtr<slang::IGlobalSession> globalSession;
  if (SLANG_FAILED(slang::createGlobalSession(globalSession.writeRef()))) {
    result.error = "failed to create Slang global session";
    return result;
  }

  slang::TargetDesc target{};
  target.format = SLANG_SPIRV;
  target.profile = globalSession->findProfile("spirv_1_0");

  std::vector<const char *> searchPaths{};
  searchPaths.reserve(desc.searchPaths.size() + (desc.path.empty() ? 0 : 1));
  for (const auto &path : desc.searchPaths) {
    searchPaths.push_back(path.c_str());
  }

  const auto slash = desc.path.find_last_of("/\\");
  std::string sourceDir{};
  if (slash != std::string::npos) {
    sourceDir = desc.path.substr(0, slash);
    searchPaths.push_back(sourceDir.c_str());
  }

  std::vector<slang::PreprocessorMacroDesc> macros{};
  macros.reserve(desc.macros.size());
  for (const auto &macro : desc.macros) {
    macros.push_back({macro.first.c_str(), macro.second.c_str()});
  }

  std::vector<slang::CompilerOptionEntry> options{};
  if (desc.generateDebugInfo) {
    slang::CompilerOptionEntry option{};
    option.name = slang::CompilerOptionName::DebugInformation;
    option.value.kind = slang::CompilerOptionValueKind::Int;
    option.value.intValue0 = SLANG_DEBUG_INFO_LEVEL_STANDARD;
    options.push_back(option);
  }

  if (desc.warningsAsErrors) {
    slang::CompilerOptionEntry option{};
    option.name = slang::CompilerOptionName::WarningsAsErrors;
    option.value.kind = slang::CompilerOptionValueKind::String;
    option.value.stringValue0 = "all";
    options.push_back(option);
  }

  slang::SessionDesc sessionDesc{};
  sessionDesc.targets = &target;
  sessionDesc.targetCount = 1;
  sessionDesc.searchPaths = searchPaths.empty() ? nullptr : searchPaths.data();
  sessionDesc.searchPathCount = static_cast<SlangInt>(searchPaths.size());
  sessionDesc.preprocessorMacros = macros.empty() ? nullptr : macros.data();
  sessionDesc.preprocessorMacroCount = static_cast<SlangInt>(macros.size());
  sessionDesc.compilerOptionEntries =
      options.empty() ? nullptr : options.data();
  sessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(options.size());
  sessionDesc.skipSPIRVValidation = desc.skipSpirvValidation;

  Slang::ComPtr<slang::ISession> session;
  if (SLANG_FAILED(
          globalSession->createSession(sessionDesc, session.writeRef()))) {
    result.error = "failed to create Slang session";
    return result;
  }

  Slang::ComPtr<ISlangBlob> diagnostics;
  Slang::ComPtr<slang::IModule> module(
      Slang::INIT_ATTACH, session->loadModuleFromSourceString(
                              desc.moduleName.c_str(), desc.label.c_str(),
                              source.c_str(), diagnostics.writeRef()));
  if (module.get() == nullptr) {
    result.error = blobString(diagnostics.get());
    if (result.error.empty()) {
      result.error = "failed to load Slang module: " + desc.label;
    }
    return result;
  }

  Slang::ComPtr<slang::IEntryPoint> entryPoint;
  diagnostics.setNull();
  if (SLANG_FAILED(module->findAndCheckEntryPoint(
          desc.entryPoint.c_str(), toSlangStage(desc.stage),
          entryPoint.writeRef(), diagnostics.writeRef()))) {
    result.error = blobString(diagnostics.get());
    if (result.error.empty()) {
      result.error = "failed to find Slang entry point: " + desc.entryPoint;
    }
    return result;
  }

  slang::IComponentType *components[] = {module.get(), entryPoint.get()};
  Slang::ComPtr<slang::IComponentType> program;
  diagnostics.setNull();
  if (SLANG_FAILED(session->createCompositeComponentType(
          components, 2, program.writeRef(), diagnostics.writeRef()))) {
    result.error = blobString(diagnostics.get());
    if (result.error.empty()) {
      result.error = "failed to compose Slang program";
    }
    return result;
  }

  Slang::ComPtr<slang::IComponentType> linkedProgram;
  diagnostics.setNull();
  if (SLANG_FAILED(
          program->link(linkedProgram.writeRef(), diagnostics.writeRef()))) {
    result.error = blobString(diagnostics.get());
    if (result.error.empty()) {
      result.error = "failed to link Slang program";
    }
    return result;
  }

  Slang::ComPtr<ISlangBlob> code;
  diagnostics.setNull();
  if (SLANG_FAILED(linkedProgram->getEntryPointCode(0, 0, code.writeRef(),
                                                    diagnostics.writeRef()))) {
    result.error = blobString(diagnostics.get());
    if (result.error.empty()) {
      result.error = "failed to generate Slang SPIR-V";
    }
    return result;
  }

  if (code.get() == nullptr || code->getBufferPointer() == nullptr ||
      code->getBufferSize() == 0 ||
      code->getBufferSize() % sizeof(uint32_t) != 0) {
    result.error = "Slang generated invalid SPIR-V bytecode";
    return result;
  }

  result.success = true;
  result.spv.resize(code->getBufferSize() / sizeof(uint32_t));
  std::memcpy(result.spv.data(), code->getBufferPointer(),
              code->getBufferSize());
  return result;
#endif
}

} // namespace vkr::util
