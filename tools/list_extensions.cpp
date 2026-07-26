#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

auto main() -> int {
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

  uint32_t instanceVersion = VK_API_VERSION_1_0;
  auto enumerateInstanceVersion =
      reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
          vkGetInstanceProcAddr(VK_NULL_HANDLE,
                                "vkEnumerateInstanceVersion"));
  if (enumerateInstanceVersion != nullptr) {
    enumerateInstanceVersion(&instanceVersion);
  }

  std::cout << "Vulkan Instance Version: "
            << VK_VERSION_MAJOR(instanceVersion) << "."
            << VK_VERSION_MINOR(instanceVersion) << "."
            << VK_VERSION_PATCH(instanceVersion) << "\n\n";

  std::cout << "Instance Extensions: count = " << extensions.size() << "\n";
  std::cout << "-------------------------------\n";
  for (const auto &extension : extensions) {
    std::cout << extension.extensionName << " : extension revision "
              << extension.specVersion << "\n";
  }

  return 0;
}
