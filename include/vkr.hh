#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "vkr/exec/render/app.hh"
#include "vkr/core/sync/fence.hh"
#include "vkr/core/sync/semaphore.hh"
#include "vkr/exec/compute/app.hh"
#include "vkr/exec/compute/graph.hh"
#include "vkr/exec/compute/executor.hh"
#include "vkr/exec/compute/pass.hh"
#include "vkr/pipeline/compute_pipeline.hh"
#include "vkr/exec/render/passes/feedback_fullscreen.hh"
#include "vkr/exec/render/passes/present.hh"
#include "vkr/resource/buffer/buffer.hh"
#include "vkr/resource/buffer/frame_uniform_buffers.hh"
#include "vkr/resource/buffer/storage_buffer.hh"
#include "vkr/resource/buffer/uniform_buffer.hh"
#include "vkr/resource/buffer/vbos.hh"
#include "vkr/resource/image/storage_image.hh"
#include "vkr/exec/render/targets/frame_history.hh"
#include "vkr/scene/geometry/mesh.hh"
#include "vkr/scene/material/cubemap.hh"
#include "vkr/scene/material/texture.hh"
#include "vkr/scene/scene.hh"
