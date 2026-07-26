#include "vk_tool_common.hh"

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace {

auto memoryPropertyFlags(VkMemoryPropertyFlags flags) -> std::string {
  std::ostringstream out{};
  if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
    out << " device-local";
  }
  if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
    out << " host-visible";
  }
  if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
    out << " host-coherent";
  }
  if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
    out << " host-cached";
  }
  if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
    out << " lazily-allocated";
  }
  if (flags == 0) {
    out << " none";
  }
  return out.str();
}

auto heapFlags(VkMemoryHeapFlags flags) -> std::string {
  std::ostringstream out{};
  if (flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
    out << " device-local";
  }
  if (flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) {
    out << " multi-instance";
  }
  if (flags == 0) {
    out << " none";
  }
  return out.str();
}

} // namespace

auto main() -> int {
  try {
    VkInstance instance = vktool::createInstance("check_memory_heaps", false);
    const auto devices = vktool::physicalDevices(instance);

    std::cout << "Memory Heaps and Types:\n";
    std::cout << "=======================\n";

    for (uint32_t deviceIndex = 0; deviceIndex < devices.size();
         ++deviceIndex) {
      VkPhysicalDeviceProperties props{};
      vkGetPhysicalDeviceProperties(devices[deviceIndex], &props);

      VkPhysicalDeviceMemoryProperties memory{};
      vkGetPhysicalDeviceMemoryProperties(devices[deviceIndex], &memory);

      std::cout << "GPU" << deviceIndex << ": " << props.deviceName << "\n";
      std::cout << "        memoryHeaps = " << memory.memoryHeapCount << "\n";
      for (uint32_t i = 0; i < memory.memoryHeapCount; ++i) {
        const double mib =
            static_cast<double>(memory.memoryHeaps[i].size) /
            (1024.0 * 1024.0);
        std::cout << "        heap[" << i << "]: size = " << mib
                  << " MiB, flags =" << heapFlags(memory.memoryHeaps[i].flags)
                  << "\n";
      }

      std::cout << "        memoryTypes = " << memory.memoryTypeCount << "\n";
      for (uint32_t i = 0; i < memory.memoryTypeCount; ++i) {
        const auto &type = memory.memoryTypes[i];
        std::cout << "        type[" << i << "]: heap = " << type.heapIndex
                  << ", flags ="
                  << memoryPropertyFlags(type.propertyFlags) << "\n";
      }
      std::cout << "\n";
    }

    vkDestroyInstance(instance, nullptr);
  } catch (const std::exception &e) {
    std::cerr << "check_memory_heaps failed: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
