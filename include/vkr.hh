#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "vkr/app.hh"
#include "vkr/core/sync/fence.hh"
#include "vkr/core/sync/semaphore.hh"
#include "vkr/pipeline/compute_pipeline.hh"
#include "vkr/render/passes/feedback_fullscreen.hh"
#include "vkr/render/passes/present.hh"
#include "vkr/resource/buffers/buffer.hh"
#include "vkr/resource/buffers/frame_uniform_buffers.hh"
#include "vkr/resource/buffers/storage_buffer.hh"
#include "vkr/resource/buffers/uniform_buffer.hh"
#include "vkr/resource/buffers/vbos.hh"
#include "vkr/resource/gpu/storage_image.hh"
#include "vkr/resource/targets/frame_history.hh"
