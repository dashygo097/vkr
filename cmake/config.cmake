# Shared configuration template for VKR and projects using its package.

# Compilation options
option(USE_CCACHE "Use ccache for compilation" ON)

# These options apply when building VKR. A package has fixed features.
if(NOT TARGET vkr::vkr)
  option(ENABLE_SLANG "Enable Slang shader compilation" ON)
  option(ENABLE_TOOLING "Build tooling" ON)
endif()
