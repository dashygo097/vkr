#include "vk_tool_common.hh"

#include <cstdlib>
#include <iostream>

namespace {

auto yes(VkBool32 value) -> const char * { return value ? "true" : "false"; }

} // namespace

auto main() -> int {
  try {
    VkInstance instance = vktool::createInstance("check_features", false);
    const auto devices = vktool::physicalDevices(instance);

    auto getFeatures2 = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2"));

    std::cout << "Device Features:\n";
    std::cout << "================\n";

    for (uint32_t deviceIndex = 0; deviceIndex < devices.size();
         ++deviceIndex) {
      VkPhysicalDeviceProperties props{};
      vkGetPhysicalDeviceProperties(devices[deviceIndex], &props);

      std::cout << "GPU" << deviceIndex << ": " << props.deviceName << "\n";

      if (getFeatures2 == nullptr) {
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(devices[deviceIndex], &features);
        std::cout << "        samplerAnisotropy              = "
                  << yes(features.samplerAnisotropy) << "\n";
        std::cout << "        shaderInt64                    = "
                  << yes(features.shaderInt64) << "\n";
        std::cout << "        shaderFloat64                  = "
                  << yes(features.shaderFloat64) << "\n";
        std::cout << "        geometryShader                 = "
                  << yes(features.geometryShader) << "\n";
        std::cout << "        tessellationShader             = "
                  << yes(features.tessellationShader) << "\n\n";
        continue;
      }

      VkPhysicalDeviceVulkan13Features features13{};
      features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

      VkPhysicalDeviceVulkan12Features features12{};
      features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
      features12.pNext = &features13;

      VkPhysicalDeviceFeatures2 features2{};
      features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
      features2.pNext = &features12;

      getFeatures2(devices[deviceIndex], &features2);
      const auto &features = features2.features;

      std::cout << "        samplerAnisotropy              = "
                << yes(features.samplerAnisotropy) << "\n";
      std::cout << "        shaderInt64                    = "
                << yes(features.shaderInt64) << "\n";
      std::cout << "        shaderFloat64                  = "
                << yes(features.shaderFloat64) << "\n";
      std::cout << "        geometryShader                 = "
                << yes(features.geometryShader) << "\n";
      std::cout << "        tessellationShader             = "
                << yes(features.tessellationShader) << "\n";
      std::cout << "        storageImageWriteWithoutFormat = "
                << yes(features.shaderStorageImageWriteWithoutFormat) << "\n";
      std::cout << "        descriptorIndexing             = "
                << yes(features12.descriptorIndexing) << "\n";
      std::cout << "        runtimeDescriptorArray         = "
                << yes(features12.runtimeDescriptorArray) << "\n";
      std::cout << "        bufferDeviceAddress            = "
                << yes(features12.bufferDeviceAddress) << "\n";
      std::cout << "        timelineSemaphore              = "
                << yes(features12.timelineSemaphore) << "\n";
      std::cout << "        shaderFloat16                  = "
                << yes(features12.shaderFloat16) << "\n";
      std::cout << "        shaderInt8                     = "
                << yes(features12.shaderInt8) << "\n";
      std::cout << "        dynamicRendering               = "
                << yes(features13.dynamicRendering) << "\n";
      std::cout << "        synchronization2               = "
                << yes(features13.synchronization2) << "\n";
      std::cout << "        maintenance4                   = "
                << yes(features13.maintenance4) << "\n\n";
    }

    vkDestroyInstance(instance, nullptr);
  } catch (const std::exception &e) {
    std::cerr << "check_features failed: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
