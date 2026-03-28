#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

auto main() -> int {
  uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> layers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

  if (layerCount == 0) {
    std::cout << "No Vulkan validation layers available.\n";
    return 0;
  }

  std::cout << "Available Vulkan validation layers (" << layerCount << "):\n";
  for (const auto &layer : layers) {
    std::cout << "  • " << layer.layerName << ": " << layer.description << "\n";
  }

  return 0;
}
