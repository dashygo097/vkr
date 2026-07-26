#include "vk_tool_common.hh"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

auto main() -> int {
  try {
    VkInstance instance = vktool::createInstance("check_queue_families",
                                                false);
    const auto devices = vktool::physicalDevices(instance);

    std::cout << "Queue Families:\n";
    std::cout << "===============\n";

    for (uint32_t deviceIndex = 0; deviceIndex < devices.size();
         ++deviceIndex) {
      VkPhysicalDeviceProperties props{};
      vkGetPhysicalDeviceProperties(devices[deviceIndex], &props);

      uint32_t count = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &count,
                                               nullptr);
      std::vector<VkQueueFamilyProperties> families(count);
      vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &count,
                                               families.data());

      std::cout << "GPU" << deviceIndex << ": " << props.deviceName << "\n";
      std::cout << "        count = " << families.size() << "\n";

      for (uint32_t familyIndex = 0; familyIndex < families.size();
           ++familyIndex) {
        const auto &family = families[familyIndex];
        std::cout << "        [" << familyIndex
                  << "] flags =" << vktool::queueFlagsString(family.queueFlags)
                  << "\n";
        std::cout << "            queueCount         = "
                  << family.queueCount << "\n";
        std::cout << "            timestampValidBits = "
                  << family.timestampValidBits << "\n";
        std::cout << "            minImageTransfer   = "
                  << family.minImageTransferGranularity.width << "x"
                  << family.minImageTransferGranularity.height << "x"
                  << family.minImageTransferGranularity.depth << "\n";
      }
      std::cout << "\n";
    }

    vkDestroyInstance(instance, nullptr);
  } catch (const std::exception &e) {
    std::cerr << "check_queue_families failed: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
