#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

void printDeviceType(VkPhysicalDeviceType type) {
  switch (type) {
  case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
    std::cout << "  Type: Integrated GPU\n";
    break;
  case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
    std::cout << "  Type: Discrete GPU\n";
    break;
  case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
    std::cout << "  Type: Virtual GPU\n";
    break;
  case VK_PHYSICAL_DEVICE_TYPE_CPU:
    std::cout << "  Type: CPU\n";
    break;
  default:
    std::cout << "  Type: Other\n";
    break;
  }
}

int main() {
  // -- Create Vulkan instance --
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Device Info";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "None";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  // Enable portability extension (for MoltenVK/macOS)
  std::vector<const char *> extensions = {
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  extensions.insert(extensions.end(), glfwExtensions,
                    glfwExtensions + glfwExtensionCount);

  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();
  createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

  VkInstance instance;
  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create Vulkan instance!");
  }

  // -- Enumerate physical devices --
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    std::cerr << "No Vulkan-compatible GPUs found.\n";
    return EXIT_FAILURE;
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  std::cout << "Found " << deviceCount << " Vulkan-compatible device(s):\n";

  for (const auto &device : devices) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    std::cout << "\n--- Device: " << props.deviceName << " ---\n";
    std::cout << "  API Version: " << VK_VERSION_MAJOR(props.apiVersion) << "."
              << VK_VERSION_MINOR(props.apiVersion) << "."
              << VK_VERSION_PATCH(props.apiVersion) << "\n";
    std::cout << "  Driver Version: " << props.driverVersion << "\n";
    printDeviceType(props.deviceType);
    std::cout << "  Vendor ID: 0x" << std::hex << props.vendorID << "\n";
    std::cout << "  Device ID: 0x" << std::hex << props.deviceID << std::dec
              << "\n";

    // Queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             queueFamilies.data());

    std::cout << "  Queue Families: " << queueFamilyCount << "\n";
    for (size_t i = 0; i < queueFamilies.size(); ++i) {
      const auto &q = queueFamilies[i];
      std::cout << "    - [" << i << "] ";
      if (q.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        std::cout << "Graphics ";
      if (q.queueFlags & VK_QUEUE_COMPUTE_BIT)
        std::cout << "Compute ";
      if (q.queueFlags & VK_QUEUE_TRANSFER_BIT)
        std::cout << "Transfer ";
      if (q.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
        std::cout << "SparseBinding ";
      std::cout << "\n";
    }

    // Limits and features
  }

  vkDestroyInstance(instance, nullptr);
  return EXIT_SUCCESS;
}
