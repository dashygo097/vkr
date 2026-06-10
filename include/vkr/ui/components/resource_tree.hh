#pragma once

#include "vkr/resources/manager.hh"
#include <imgui.h>
#include <string>
#include <vector>

namespace vkr::ui {

class ResourceTree {
public:
  explicit ResourceTree(const resource::ResourceManager &resourceManager);
  ~ResourceTree() = default;

  void render();

private:
  const resource::ResourceManager &resource_manager_;

  std::string selected_type_;
  std::string selected_name_;
  bool show_empty_groups_{true};

  void renderCategory(const char *type, std::vector<std::string> names,
                      size_t count);
  void renderSelectedResource();
};

} // namespace vkr::ui
