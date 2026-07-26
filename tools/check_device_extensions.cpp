#include "vk_tool_common.hh"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

auto main() -> int {
  try {
    VkInstance instance = vktool::createInstance("check_device_extensions",
                                                false);
    const auto devices = vktool::physicalDevices(instance);

    std::cout << "Device Extensions:\n";
    std::cout << "==================\n";

    for (uint32_t deviceIndex = 0; deviceIndex < devices.size();
         ++deviceIndex) {
      VkPhysicalDeviceProperties props{};
      vkGetPhysicalDeviceProperties(devices[deviceIndex], &props);

      uint32_t count = 0;
      if (vkEnumerateDeviceExtensionProperties(devices[deviceIndex], nullptr,
                                               &count, nullptr) !=
          VK_SUCCESS) {
        throw std::runtime_error("failed to enumerate device extension count");
      }

      std::vector<VkExtensionProperties> extensions(count);
      if (vkEnumerateDeviceExtensionProperties(devices[deviceIndex], nullptr,
                                               &count, extensions.data()) !=
          VK_SUCCESS) {
        throw std::runtime_error("failed to enumerate device extensions");
      }

      std::cout << "GPU" << deviceIndex << ": " << props.deviceName << "\n";
      std::cout << "        count = " << extensions.size() << "\n";
      for (const auto &extension : extensions) {
        std::cout << "        " << extension.extensionName
                  << " : extension revision " << extension.specVersion
                  << "\n";
      }
      std::cout << "\n";
    }

    vkDestroyInstance(instance, nullptr);
  } catch (const std::exception &e) {
    std::cerr << "check_device_extensions failed: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
