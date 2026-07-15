# VKR

VKR is a small graph-based Vulkan renderer and application framework written in
C++17. It wraps common Vulkan setup, render pass orchestration, resource
management, shader compilation, and an ImGui workspace so sample applications can
focus on scene resources and render graph composition.

![demo screenshot](assets/screenshots/demo.png)

## Features

- Vulkan application shell for window creation, instance/device/swapchain setup,
  command buffers, frame synchronization, resize handling, and snapshot
  persistence.
- Render graph with named passes, resource read/write tracking, dependency
  compilation, and pass recreation on swapchain changes.
- Built-in render passes for raster rendering, skyboxes, fullscreen
  post-processing, ImGui UI composition, and presentation.
- Resource management for meshes, textures, cubemaps, uniform buffers, Vulkan
  images, samplers, framebuffers, and descriptor sets.
- Runtime GLSL compilation through `shaderc`, with helpers for GLSL files and
  source strings.
- OBJ loading through `tinyobjloader`, image loading through `stb`, math through
  `glm`, logging through `spdlog`, and configuration snapshots through
  `toml++`.
- ImGui workspace with viewport, render graph, resources, assets, camera,
  performance, logging, shader editor, mesh editor, and theme controls.
- Sample applications for a textured teapot, a debug skybox, and a ShaderToy-like
  fullscreen shader viewer.
- Vulkan diagnostic tools for instance extensions, validation layers, and
  physical devices.

## Requirements

- CMake 3.24 or newer.
- A C++17 compiler.
- Vulkan SDK with `shaderc_combined`.
- Ninja is the default generator used by the Makefile.
- `ccache` is optional and used automatically when found.
- macOS builds require the Vulkan SDK with MoltenVK. The CMake files link the
  required Apple frameworks for MoltenVK.

Most third-party libraries are vendored in `3rdparty/`; the Vulkan SDK is the
main external dependency.

## Build

The repository includes a small Makefile around CMake. The first build copies
`cmake/config.cmake` to `build/config.cmake`, configures CMake, and builds the
library, tools, and sample applications.

```sh
make
```

Common variants:

```sh
make BUILD_TYPE=Release
make GENERATOR="Unix Makefiles"
make reconfigure
make clean
make distclean
```

The default generator is `Ninja`, and the default build type is `Debug`.

You can also run CMake directly:

```sh
mkdir -p build
cp cmake/config.cmake build/config.cmake
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Configuration

Local configuration values live in `build/config.cmake` after the first
`make config` or `make` run. The current template contains:

```cmake
set(USE_CCACHE ON)
set(ENABLE_UNITY_BUILD OFF)
set(ENABLE_TOOLING ON)
set(ENABLE_EXAMPLES ON)
```

Edit `build/config.cmake` for local build settings, then run:

```sh
make reconfigure
make
```

`cmake/config.cmake` is the template used when `build/config.cmake` does not
already exist.

## Run

Sample applications are written to `build/bin/<app-name>/`:

```sh
./build/bin/teapot/teapot
./build/bin/skybox/skybox
./build/bin/shadertoy/shadertoy
```

Tools are written to `build/bin/`:

```sh
./build/bin/list_extensions
./build/bin/list_validation_layers
./build/bin/check_physical_devices
```

## Examples

- `teapot`: loads `examples/teapot/assets/objects/teapot/teapot.obj`, applies a
  texture, renders it through a raster pass, and runs a fullscreen post-process
  pass before presenting through the UI pass.
- `skybox`: creates temporary debug cubemap faces, renders a skybox cube, and
  displays it through the UI pass.
- `shadertoy`: renders a fullscreen triangle using ShaderToy-style uniforms such
  as resolution, time, frame count, mouse state, and date.

Each directory under `examples/` has its own `CMakeLists.txt` and uses
`add_vk_app(...)`. If an example has an `assets/` directory, the helper copies it
to that example's output directory after build.

## Controls

- `Tab`: switch between fullscreen viewport mode and the docked workspace.
- `W`, `A`, `S`, `D`: move the camera horizontally.
- `Space`: move up.
- `Left Shift`: move down.
- Hold left mouse button and move the mouse: look around when the camera is
  unlocked.
- Mouse wheel: adjust field of view.

The docked workspace exposes camera locking and tuning controls. When the
workspace is active and the viewport is not focused, the camera is locked.

## Project Layout

```text
examples/    Sample applications and example-local assets.
assets/      Engine-level shaders and shared screenshots.
cmake/       Build configuration, dependency, compiler, and target helpers.
include/     Public VKR headers.
lib/         VKR implementation.
tools/       Small Vulkan diagnostic executables.
3rdparty/    Vendored third-party dependencies.
doc/         Placeholder project notes.
```

## Development Notes

- Include `vkr.hh` from applications to get the main application framework,
  common render passes, and built-in vertex/uniform buffer types.
- Derive from `vkr::VulkanApplication` and implement `configure()`,
  `createResources()`, `buildRenderGraph()`, and optionally `onDraw()`.
- Add a new example with a local `CMakeLists.txt`:

  ```cmake
  add_vk_app(my_example
    SOURCES
      main.cpp
    ASSET_DIR
      assets
  )
  ```

  Then add the example directory from `examples/CMakeLists.txt`.
- Use `assetSystem->resolve(...)` to load app assets first and then engine
  assets. Explicit prefixes are also available: `app://`, `engine://`, and
  `user://`.
- The application saves and loads `snapshot.toml` by default, preserving runtime
  context such as camera and UI/theme settings.

## License

Copyright 2025-2026 dashygo097

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for
details.

## References

- [Vulkan Tutorial](https://github.com/Overv/VulkanTutorial.git)
