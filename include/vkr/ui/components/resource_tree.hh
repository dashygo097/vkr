#pragma once

#include "vkr/resource/manager.hh"
#include "vkr/ui/components/ui_component.hh"
#include <imgui.h>
#include <string>
#include <vector>

namespace vkr::ui {

class ResourceTree final : public UiComponent {
public:
  explicit ResourceTree(resource::ResourceManager &resourceManager);
  ~ResourceTree() = default;

private:
  void render();

  resource::ResourceManager &resource_manager_;

  std::string selected_type_;
  std::string selected_name_;
  bool show_empty_groups_{true};

  void renderCategory(const char *type, std::vector<std::string> names,
                      size_t count);
  void renderSelectedResource();
};

} // namespace vkr::ui
