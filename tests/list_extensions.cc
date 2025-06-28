#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

int main() {
  uint32_t extensionCount = 0;

  VkResult result =
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  if (result != VK_SUCCESS) {
    std::cerr << "Failed to enumerate instance extension count!\n";
    return 1;
  }

  std::vector<VkExtensionProperties> extensions(extensionCount);

  result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                                  extensions.data());
  if (result != VK_SUCCESS) {
    std::cerr << "Failed to enumerate instance extensions!\n";
    return 1;
  }

  std::cout << "Available Vulkan Instance Extensions:\n";
  for (const auto &extension : extensions) {
    std::cout << '\t' << extension.extensionName << " (version "
              << extension.specVersion << ")\n";
  }

  return 0;
}
