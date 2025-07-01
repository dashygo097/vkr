#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class Images {
public:
  Images();
  ~Images();

  // components
  std::vector<VkImage> images;
  VkFormat format;
};
