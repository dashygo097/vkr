#include "vkr/ui/components/resource_tree.hh"
#include <algorithm>

namespace vkr::ui {

ResourceTree::ResourceTree(resource::ResourceManager &resourceManager)
    : resource_manager_(resourceManager) {}

void ResourceTree::render() {
  ImGui::TextUnformatted("Resource Tree");
  ImGui::Separator();

  ImGui::Checkbox("Show empty groups", &show_empty_groups_);
  ImGui::Spacing();

  if (ImGui::BeginChild("ResourceTreeScrollRegion", ImVec2(0.0f, -90.0f), true,
                        ImGuiWindowFlags_HorizontalScrollbar)) {
    renderCategory("Meshes", resource_manager_.listMeshNames(),
                   resource_manager_.meshCount());

    renderCategory("Uniform Buffers",
                   resource_manager_.listUniformBufferNames(),
                   resource_manager_.uniformBufferCount());

    renderCategory("Textures", resource_manager_.listTextureImageNames(),
                   resource_manager_.textureImageCount());
  }

  ImGui::EndChild();

  renderSelectedResource();
}

void ResourceTree::renderCategory(const char *type,
                                  std::vector<std::string> names,
                                  size_t count) {
  names.erase(std::remove_if(
                  names.begin(), names.end(),
                  [](const std::string &name) -> bool { return name.empty(); }),
              names.end());

  std::sort(names.begin(), names.end());
  names.erase(std::unique(names.begin(), names.end()), names.end());

  if (!show_empty_groups_ && names.empty()) {
    return;
  }

  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

  if (names.empty()) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }

  const bool open = ImGui::TreeNodeEx(type, flags, "%s (%zu)", type, count);

  if (open) {
    if (names.empty()) {
      ImGui::TextDisabled("No resources");
    } else {
      for (const auto &name : names) {
        ImGui::PushID(name.c_str());

        const bool selected = selected_type_ == type && selected_name_ == name;

        if (ImGui::Selectable(name.c_str(), selected)) {
          selected_type_ = type;
          selected_name_ = name;

          if (selected_type_ == "Meshes") {
            resource_manager_.selectMesh(selected_name_);
          } else {
            resource_manager_.clearSelectedMesh();
          }
        }

        if (ImGui::BeginPopupContextItem("ResourceContextMenu")) {
          ImGui::TextDisabled("%s", type);
          ImGui::Separator();

          if (ImGui::MenuItem("Copy name")) {
            ImGui::SetClipboardText(name.c_str());
          }

          ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          ImGui::TextUnformatted(type);
          ImGui::Separator();
          ImGui::Text("%s", name.c_str());
          ImGui::EndTooltip();
        }

        ImGui::PopID();
      }
    }

    ImGui::TreePop();
  }
}

void ResourceTree::renderSelectedResource() {
  ImGui::Separator();

  ImGui::TextUnformatted("Selected Resource");

  if (selected_name_.empty()) {
    ImGui::TextDisabled("None");
    return;
  }

  ImGui::Text("Type: %s", selected_type_.c_str());
  ImGui::Text("Name: %s", selected_name_.c_str());

  if (ImGui::Button("Copy name")) {
    ImGui::SetClipboardText(selected_name_.c_str());
  }
}

} // namespace vkr::ui
