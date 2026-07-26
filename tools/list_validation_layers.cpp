#include <iostream>
#include <sstream>
#include <vector>
#include <vulkan/vulkan.h>

namespace {

auto versionString(uint32_t version) -> std::string {
  std::ostringstream out{};
  out << VK_VERSION_MAJOR(version) << "." << VK_VERSION_MINOR(version) << "."
      << VK_VERSION_PATCH(version);
  return out.str();
}

} // namespace

auto main() -> int {
  uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> layers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

  std::cout << "Instance Layers: count = " << layerCount << "\n";
  std::cout << "---------------------------\n";
  for (const auto &layer : layers) {
    std::cout << layer.layerName << " " << layer.description << " "
              << versionString(layer.specVersion) << " version "
              << layer.implementationVersion << "\n";
  }

  return 0;
}
