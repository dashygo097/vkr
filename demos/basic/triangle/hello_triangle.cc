#include <vulkan/vulkan.h>

class HelloTriangleApplication {
public:
  void run() {
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  void initVulkan() {
    // Initialize Vulkan instance, devices, etc.
  }
  void mainLoop() {
    // Main rendering loop
  }
  void cleanup() {
    // Cleanup Vulkan resources
  }
};
