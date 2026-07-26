#include "vk_tool_common.hh"

#include <cstdlib>
#include <iostream>

auto main() -> int {
  try {
    VkInstance instance = vktool::createInstance("check_limits", false);
    const auto devices = vktool::physicalDevices(instance);

    std::cout << "Device Limits:\n";
    std::cout << "==============\n";

    for (uint32_t deviceIndex = 0; deviceIndex < devices.size();
         ++deviceIndex) {
      VkPhysicalDeviceProperties props{};
      vkGetPhysicalDeviceProperties(devices[deviceIndex], &props);
      const auto &limits = props.limits;

      std::cout << "GPU" << deviceIndex << ": " << props.deviceName << "\n";
      std::cout << "        maxImageDimension2D             = "
                << limits.maxImageDimension2D << "\n";
      std::cout << "        maxUniformBufferRange           = "
                << limits.maxUniformBufferRange << "\n";
      std::cout << "        maxStorageBufferRange           = "
                << limits.maxStorageBufferRange << "\n";
      std::cout << "        minUniformBufferOffsetAlignment = "
                << limits.minUniformBufferOffsetAlignment << "\n";
      std::cout << "        minStorageBufferOffsetAlignment = "
                << limits.minStorageBufferOffsetAlignment << "\n";
      std::cout << "        maxComputeSharedMemorySize      = "
                << limits.maxComputeSharedMemorySize << "\n";
      std::cout << "        maxComputeWorkGroupInvocations  = "
                << limits.maxComputeWorkGroupInvocations << "\n";
      std::cout << "        maxComputeWorkGroupCount        = "
                << limits.maxComputeWorkGroupCount[0] << "x"
                << limits.maxComputeWorkGroupCount[1] << "x"
                << limits.maxComputeWorkGroupCount[2] << "\n";
      std::cout << "        maxComputeWorkGroupSize         = "
                << limits.maxComputeWorkGroupSize[0] << "x"
                << limits.maxComputeWorkGroupSize[1] << "x"
                << limits.maxComputeWorkGroupSize[2] << "\n";
      std::cout << "        timestampPeriod                 = "
                << limits.timestampPeriod << " ns\n";
      std::cout << "        framebufferColorSampleCounts    = 0x"
                << std::hex << limits.framebufferColorSampleCounts << std::dec
                << "\n";
      std::cout << "        framebufferDepthSampleCounts    = 0x"
                << std::hex << limits.framebufferDepthSampleCounts << std::dec
                << "\n\n";
    }

    vkDestroyInstance(instance, nullptr);
  } catch (const std::exception &e) {
    std::cerr << "check_limits failed: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
