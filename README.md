# VKR

VKR is a small C++17 Vulkan framework for graphics and compute experiments. It
wraps common Vulkan setup, command pools, device capability queries, resource
ownership, descriptor setup, shader compilation, and graph-style execution so
examples can focus on rendering or compute work.

![demo screenshot](assets/screenshots/demo.png)

## Current Shape

The framework is split by responsibility:

- `core`: Vulkan instance, physical/logical device selection, queue-family
  support queries, command pools, windows, surfaces, swapchains, fences, and
  semaphores.
- `resource`: low-level GPU resource ownership such as buffers, storage
  buffers, uniform buffers, images, image views, samplers, storage images, and
  shader modules. These types do not own render-frame concepts.
- `scene`: graphics-facing scene resources such as meshes, vertex/index
  buffers, vertex input layouts, textures, cubemaps, cameras, and frame uniform
  buffer sets.
- `pipeline`: descriptor layouts, descriptor sets, descriptor pools, graphics
  pipelines, compute pipelines, and render-pass wrappers.
- `exec::render`: render application shell, render graph, render executor,
  render targets, attachments, frame sync, and built-in render passes.
- `exec::compute`: compute application shell, compute graph, compute executor,
  and compute passes.
- `exec::profiler`: GPU timestamp profiling infrastructure shared by execution
  systems.

Applications usually include `vkr.hh`, then derive from either
`vkr::exec::RenderApplication` or `vkr::exec::ComputeApplication`.

## Features

- Vulkan instance/device setup with queried graphics, present, compute, and
  transfer queue-family support.
- Separate render and compute application shells.
- Render graph with named passes, read/write tracking, dependency compilation,
  swapchain recreation, and frame synchronization.
- Compute graph with descriptor-backed compute passes and dispatch execution.
- GPU timestamp profiler for pass-level compute profiling when the selected
  queue family supports timestamp queries.
- Built-in render passes for raster rendering, skyboxes, fullscreen passes,
  feedback fullscreen passes, ImGui UI composition, and presentation.
- Generic resource types for buffers, storage buffers, uniform buffers, images,
  storage images, image views, samplers, and shader modules.
- Scene-layer graphics resources for meshes, vertex/index buffers, textures,
  cubemaps, cameras, and frame uniform buffer sets.
- Runtime GLSL compilation through `shaderc`, with helpers for GLSL files and
  source strings.
- OBJ loading through `tinyobjloader`, image loading through `stb`, math through
  `glm`, logging through `spdlog`, and snapshots through `toml++`.
- ImGui workspace with viewport, exec graph, resources, assets, camera,
  performance, logging, shader editor, mesh editor, and theme controls.
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

The repository includes a Makefile wrapper around CMake. The first build copies
`cmake/config.cmake` to `build/config.cmake`, configures CMake, and builds the
library, tools, and examples.

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

Examples are written to `build/bin/<app-name>/`:

```sh
./build/bin/teapot/teapot
./build/bin/skybox/skybox
./build/bin/shadertoy/shadertoy
./build/bin/vector_add/vector_add
```

Tools are written to `build/bin/`:

```sh
./build/bin/list_extensions
./build/bin/list_validation_layers
./build/bin/check_physical_devices
```

On macOS, compute and render examples still require a working MoltenVK/Metal
runtime.

## Examples

- `teapot`: render example that loads an OBJ teapot, creates scene textures and
  mesh buffers, renders through a raster pass, applies a fullscreen pass, then
  presents through the UI pass.
- `skybox`: render example that creates a cubemap and renders a skybox.
- `shadertoy`: render example with ShaderToy-style fullscreen feedback passes,
  uniforms for time, frame count, mouse state, date, and a shader editor.
- `vector_add`: compute example that creates storage buffers and a uniform
  buffer, binds them to a compute pass, dispatches a GLSL compute shader, then
  reads back and validates the result.

Each directory under `examples/` has its own `CMakeLists.txt` and uses
`add_vk_app(...)`. If an example has an `assets/` directory, the helper copies it
to that example's output directory after build.

## Render App Skeleton

```cpp
#include "vkr.hh"

class MyRenderApp final : public vkr::exec::RenderApplication {
  void configure() override {
    ctx.window.title = "my_render_app";
    ctx.instance.name = "my_render_app";
  }

  void createResources() override {
    // Create scene meshes, textures, cubemaps, and frame uniform buffers.
  }

  void buildGraph() override {
    // Add render passes to graph.
  }
};

int main() {
  MyRenderApp app;
  app.run();
}
```

Render applications own a window, surface, swapchain, scene, render executor,
render graph, graphics command pool, and optional compute/transfer command pools
when supported by the selected device.

## Compute App Skeleton

```cpp
#include "vkr.hh"

class MyComputeApp final : public vkr::exec::ComputeApplication {
  void configure() override {
    ctx.instance.name = "my_compute_app";
  }

  void createResources() override {
    // Create resource::StorageBuffer<T>, resource::UniformBuffer<T>, etc.
  }

  void buildGraph() override {
    // Add compute passes to graph.
  }

  void afterExecute() override {
    // Read back and validate results if needed.
  }
};

int main() {
  MyComputeApp app;
  app.run();
}
```

For compute uniforms, create a generic `vkr::resource::UniformBuffer<T>` and
bind `uniform.descriptorInfo()` with `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`.
For input/output arrays, use `vkr::resource::StorageBuffer<T>` and bind
`storage.descriptorInfo(...)` with `VK_DESCRIPTOR_TYPE_STORAGE_BUFFER`.

`ComputeApplication` creates an `exec::Profiler` by default. During execution it
wraps each compute pass in a GPU timestamp scope and stores the result in
`profileReport`. The report is also logged after the command buffer completes:

```text
GPU profile report:
  vector_add: 0.012345 ms
```

Profiling can be configured from `ctx.profiler`:

```cpp
void configure() override {
  ctx.instance.name = "my_compute_app";
  ctx.profiler.enableGpuTimestamps = true;
  ctx.profiler.maxScopes = 64;
}
```

If the compute queue family reports `timestampValidBits == 0`, the profiler is
disabled with a warning and the app still runs normally.

## Controls

Render examples use these default controls:

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
examples/    Render and compute sample applications.
assets/      Engine-level shaders and shared screenshots.
cmake/       Build configuration, dependency, compiler, and target helpers.
include/     Public VKR headers.
lib/         VKR implementation.
tools/       Small Vulkan diagnostic executables.
3rdparty/    Vendored third-party dependencies.
doc/         Placeholder project notes.
```

Important public header groups:

```text
include/vkr/core/       Vulkan setup, command pools, sync, window/surface.
include/vkr/resource/   Low-level buffers, images, samplers, shader modules.
include/vkr/scene/      Graphics scene resources and frame uniform sets.
include/vkr/pipeline/   Descriptor and pipeline wrappers.
include/vkr/exec/       Render and compute execution frameworks.
include/vkr/exec/profiler/
                        GPU timestamp profiler.
include/vkr/ui/         ImGui workspace components.
include/vkr/util/       Assets, compiler, timer, input, and TOML utilities.
```

## Development Notes

- Use `vkr.hh` from examples and applications for the main public API.
- Public source files include their own headers normally; the library target
  uses `include/vkr/pch.hh` as a private precompiled header.
- Keep low-level `resource` types free of render-frame and scene concepts.
  Graphics-specific vertex/index buffers, meshes, textures, and cubemaps belong
  in `scene`.
- Use `resource::UniformBuffer<T>` for a single generic uniform buffer.
  Use `scene::FrameUniformBufferSet<T>` only when a render pass needs one
  uniform buffer per frame in flight.
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

- Use `assetSystem->resolveApp(...)` for example-local assets. The asset system
  also supports engine and user roots.
- Render applications save and load `snapshot.toml` by default, preserving
  runtime context such as camera and UI/theme settings.

## License

Copyright 2025-2026 dashygo097

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for
details.

## References

- [Vulkan Tutorial](https://github.com/Overv/VulkanTutorial.git)
