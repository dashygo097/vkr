#include "vkr/ui/components/resource_tree.hh"
#include <algorithm>
#include <utility>

namespace vkr::ui {

ResourceTree::ResourceTree(const scene::Scene &scene,
                           const Selection &selection,
                           std::function<void(Selection)> onSelect)
    : UiComponent("Resources"), scene_(scene), selection_(selection),
      on_select_(std::move(onSelect)) {}

void ResourceTree::render() {
  ImGui::SetNextItemWidth(-1.0f);
  if (ImGui::InputTextWithHint("##resource_filter", "Filter resources...",
                               filter_.InputBuf, sizeof(filter_.InputBuf))) {
    filter_.Build();
  }
  ImGui::Checkbox("Empty groups", &show_empty_groups_);

  if (ImGui::BeginChild("ResourceTreeScrollRegion", ImVec2(0.0f, 0.0f),
                        ImGuiChildFlags_None,
                        ImGuiWindowFlags_HorizontalScrollbar)) {
    renderCategory(SelectionType::Mesh, "Meshes", scene_.listMeshNames(),
                   scene_.meshCount());

    renderCategory(SelectionType::UniformBuffer, "Uniform Buffers",
                   scene_.listUniformBufferNames(),
                   scene_.uniformBufferCount());

    renderCategory(SelectionType::Texture, "Textures",
                   scene_.listTextureNames(), scene_.textureCount());

    renderCategory(SelectionType::Cubemap, "Cubemaps",
                   scene_.listCubemapNames(), scene_.cubemapCount());
  }

  ImGui::EndChild();
}

void ResourceTree::renderCategory(SelectionType type, std::string_view label,
                                  std::vector<std::string> names,
                                  size_t count) {
  names.erase(std::remove_if(names.begin(), names.end(),
                             [this](const std::string &name) -> bool {
                               return name.empty() ||
                                      !filter_.PassFilter(name.c_str());
                             }),
              names.end());

  std::sort(names.begin(), names.end());
  names.erase(std::unique(names.begin(), names.end()), names.end());

  if (names.empty() && (!show_empty_groups_ || filter_.IsActive())) {
    return;
  }

  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

  if (names.empty()) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }

  const bool open =
      ImGui::TreeNodeEx(label.data(), flags, "%s (%zu)", label.data(), count);

  if (open) {
    if (names.empty()) {
      ImGui::TextDisabled("No resources");
    } else {
      for (const auto &name : names) {
        ImGui::PushID(name.c_str());

        const bool selected =
            selection_.type == type && selection_.name == name;
        if (ImGui::Selectable(name.c_str(), selected)) {
          on_select_({type, name});
        }

        if (ImGui::BeginPopupContextItem("ResourceContextMenu")) {
          ImGui::TextDisabled("%s", label.data());
          ImGui::Separator();

          if (ImGui::MenuItem("Copy name")) {
            ImGui::SetClipboardText(name.c_str());
          }

          ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          ImGui::TextUnformatted(label.data(), label.data() + label.size());
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

} // namespace vkr::ui
