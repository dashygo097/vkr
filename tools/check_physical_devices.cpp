#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <vulkan/vulkan_beta.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

auto versionString(uint32_t version) -> std::string {
  std::ostringstream out{};
  out << VK_VERSION_MAJOR(version) << "." << VK_VERSION_MINOR(version) << "."
      << VK_VERSION_PATCH(version);
  return out.str();
}

auto deviceTypeString(VkPhysicalDeviceType type) -> const char * {
  switch (type) {
  case VK_PHYSICAL_DEVICE_TYPE_OTHER:
    return "PHYSICAL_DEVICE_TYPE_OTHER";
  case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
    return "PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
  case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
    return "PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
  case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
    return "PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
  case VK_PHYSICAL_DEVICE_TYPE_CPU:
    return "PHYSICAL_DEVICE_TYPE_CPU";
  default:
    return "PHYSICAL_DEVICE_TYPE_UNKNOWN";
  }
}

auto driverIdString(VkDriverId driverId) -> const char * {
  switch (driverId) {
  case VK_DRIVER_ID_AMD_PROPRIETARY:
    return "DRIVER_ID_AMD_PROPRIETARY";
  case VK_DRIVER_ID_AMD_OPEN_SOURCE:
    return "DRIVER_ID_AMD_OPEN_SOURCE";
  case VK_DRIVER_ID_MESA_RADV:
    return "DRIVER_ID_MESA_RADV";
  case VK_DRIVER_ID_NVIDIA_PROPRIETARY:
    return "DRIVER_ID_NVIDIA_PROPRIETARY";
  case VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS:
    return "DRIVER_ID_INTEL_PROPRIETARY_WINDOWS";
  case VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA:
    return "DRIVER_ID_INTEL_OPEN_SOURCE_MESA";
  case VK_DRIVER_ID_IMAGINATION_PROPRIETARY:
    return "DRIVER_ID_IMAGINATION_PROPRIETARY";
  case VK_DRIVER_ID_QUALCOMM_PROPRIETARY:
    return "DRIVER_ID_QUALCOMM_PROPRIETARY";
  case VK_DRIVER_ID_ARM_PROPRIETARY:
    return "DRIVER_ID_ARM_PROPRIETARY";
  case VK_DRIVER_ID_GOOGLE_SWIFTSHADER:
    return "DRIVER_ID_GOOGLE_SWIFTSHADER";
  case VK_DRIVER_ID_GGP_PROPRIETARY:
    return "DRIVER_ID_GGP_PROPRIETARY";
  case VK_DRIVER_ID_BROADCOM_PROPRIETARY:
    return "DRIVER_ID_BROADCOM_PROPRIETARY";
  case VK_DRIVER_ID_MESA_LLVMPIPE:
    return "DRIVER_ID_MESA_LLVMPIPE";
  case VK_DRIVER_ID_MOLTENVK:
    return "DRIVER_ID_MOLTENVK";
  case VK_DRIVER_ID_COREAVI_PROPRIETARY:
    return "DRIVER_ID_COREAVI_PROPRIETARY";
  case VK_DRIVER_ID_JUICE_PROPRIETARY:
    return "DRIVER_ID_JUICE_PROPRIETARY";
  case VK_DRIVER_ID_VERISILICON_PROPRIETARY:
    return "DRIVER_ID_VERISILICON_PROPRIETARY";
  case VK_DRIVER_ID_MESA_TURNIP:
    return "DRIVER_ID_MESA_TURNIP";
  case VK_DRIVER_ID_MESA_V3DV:
    return "DRIVER_ID_MESA_V3DV";
  case VK_DRIVER_ID_MESA_PANVK:
    return "DRIVER_ID_MESA_PANVK";
  case VK_DRIVER_ID_SAMSUNG_PROPRIETARY:
    return "DRIVER_ID_SAMSUNG_PROPRIETARY";
  case VK_DRIVER_ID_MESA_VENUS:
    return "DRIVER_ID_MESA_VENUS";
  case VK_DRIVER_ID_MESA_DOZEN:
    return "DRIVER_ID_MESA_DOZEN";
  case VK_DRIVER_ID_MESA_NVK:
    return "DRIVER_ID_MESA_NVK";
  case VK_DRIVER_ID_IMAGINATION_OPEN_SOURCE_MESA:
    return "DRIVER_ID_IMAGINATION_OPEN_SOURCE_MESA";
  default:
    return "DRIVER_ID_UNKNOWN";
  }
}

auto uuidString(const uint8_t uuid[VK_UUID_SIZE]) -> std::string {
  std::ostringstream out{};
  out << std::hex << std::setfill('0');
  for (uint32_t i = 0; i < VK_UUID_SIZE; ++i) {
    out << std::setw(2) << static_cast<uint32_t>(uuid[i]);
    if (i == 3 || i == 5 || i == 7 || i == 9) {
      out << "-";
    }
  }
  return out.str();
}

auto conformanceString(const VkConformanceVersion &version) -> std::string {
  std::ostringstream out{};
  out << static_cast<uint32_t>(version.major) << "."
      << static_cast<uint32_t>(version.minor) << "."
      << static_cast<uint32_t>(version.subminor) << "."
      << static_cast<uint32_t>(version.patch);
  return out.str();
}

auto instanceExtensions() -> std::vector<VkExtensionProperties> {
  uint32_t count = 0;
  if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to enumerate instance extension count");
  }

  std::vector<VkExtensionProperties> extensions(count);
  if (vkEnumerateInstanceExtensionProperties(nullptr, &count,
                                             extensions.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to enumerate instance extensions");
  }

  return extensions;
}

auto hasExtension(const std::vector<VkExtensionProperties> &extensions,
                  const char *name) -> bool {
  return std::any_of(extensions.begin(), extensions.end(),
                     [name](const VkExtensionProperties &extension) {
                       return std::strcmp(extension.extensionName, name) == 0;
                     });
}

auto createInstance(const std::vector<VkExtensionProperties> &available)
    -> VkInstance {
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "check_physical_devices";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "None";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  std::vector<const char *> extensions{};
#ifdef __APPLE__
  if (hasExtension(available, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  }
  if (hasExtension(available,
                   VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  }
#endif

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames =
      extensions.empty() ? nullptr : extensions.data();

#ifdef __APPLE__
  if (hasExtension(available, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }
#endif

  VkInstance instance = VK_NULL_HANDLE;
  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create Vulkan instance");
  }

  return instance;
}

void printQueues(VkPhysicalDevice device) {
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

  std::vector<VkQueueFamilyProperties> families(count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

  std::cout << "        queueFamilies      = " << count << "\n";
  for (uint32_t i = 0; i < count; ++i) {
    const auto &family = families[i];
    std::cout << "          [" << i << "] flags =";
    if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      std::cout << " graphics";
    }
    if (family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
      std::cout << " compute";
    }
    if (family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
      std::cout << " transfer";
    }
    if (family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
      std::cout << " sparse";
    }
    if (family.queueFlags == 0) {
      std::cout << " none";
    }
    std::cout << ", count = " << family.queueCount
              << ", timestampValidBits = " << family.timestampValidBits
              << "\n";
  }
}

} // namespace

auto main() -> int {
  try {
    const auto extensions = instanceExtensions();
    VkInstance instance = createInstance(extensions);

    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to enumerate physical device count");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    if (deviceCount > 0 &&
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()) !=
            VK_SUCCESS) {
      throw std::runtime_error("failed to enumerate physical devices");
    }

    std::cout << "Devices:\n";
    std::cout << "========\n";

    auto getProperties2 =
        reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
            vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2"));

    for (uint32_t i = 0; i < deviceCount; ++i) {
      VkPhysicalDeviceProperties props{};
      VkPhysicalDeviceDriverProperties driver{};
      VkPhysicalDeviceIDProperties id{};

      if (getProperties2 != nullptr) {
        driver.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
        id.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
        driver.pNext = &id;

        VkPhysicalDeviceProperties2 props2{};
        props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        props2.pNext = &driver;
        getProperties2(devices[i], &props2);
        props = props2.properties;
      } else {
        vkGetPhysicalDeviceProperties(devices[i], &props);
      }

      std::cout << "GPU" << i << ":\n";
      std::cout << "        apiVersion         = "
                << versionString(props.apiVersion) << "\n";
      std::cout << "        driverVersion      = " << props.driverVersion
                << "\n";
      std::cout << "        vendorID           = 0x" << std::hex
                << props.vendorID << std::dec << "\n";
      std::cout << "        deviceID           = 0x" << std::hex
                << props.deviceID << std::dec << "\n";
      std::cout << "        deviceType         = "
                << deviceTypeString(props.deviceType) << "\n";
      std::cout << "        deviceName         = " << props.deviceName << "\n";

      if (getProperties2 != nullptr) {
        std::cout << "        driverID           = "
                  << driverIdString(driver.driverID) << "\n";
        std::cout << "        driverName         = " << driver.driverName
                  << "\n";
        std::cout << "        driverInfo         = " << driver.driverInfo
                  << "\n";
        std::cout << "        conformanceVersion = "
                  << conformanceString(driver.conformanceVersion) << "\n";
        std::cout << "        deviceUUID         = "
                  << uuidString(id.deviceUUID) << "\n";
        std::cout << "        driverUUID         = "
                  << uuidString(id.driverUUID) << "\n";
      }

      printQueues(devices[i]);
      std::cout << "\n";
    }

    vkDestroyInstance(instance, nullptr);
  } catch (const std::exception &e) {
    std::cerr << "check_physical_devices failed: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
