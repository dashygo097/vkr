#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "vkr/app.hh"
#include "vkr/render/passes/feedback_fullscreen.hh"
#include "vkr/render/passes/present.hh"
#include "vkr/resource/buffers/ubos.hh"
#include "vkr/resource/buffers/vbos.hh"
#include "vkr/resource/targets/ping_pong.hh"
