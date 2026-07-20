#include "vkr/ui/components/resource_tree.hh"
#include <algorithm>

namespace vkr::ui {

ResourceTree::ResourceTree(resource::ResourceManager &resourceManager)
    : UiComponent("Resources"), resource_manager_(resourceManager) {}

void ResourceTree::render() {
  ImGui::TextUnformatted("Resource Tree");
  ImGui::Separator();

  ImGui::Checkbox("Show empty groups", &show_empty_groups_);
  ImGui::Spacing();

  const float detailsHeight = ImGui::GetTextLineHeightWithSpacing() * 11.0f;
  if (ImGui::BeginChild("ResourceTreeScrollRegion",
                        ImVec2(0.0f, -detailsHeight), true,
                        ImGuiWindowFlags_HorizontalScrollbar)) {
    renderCategory("Meshes", resource_manager_.listMeshNames(),
                   resource_manager_.meshCount());

    renderCategory("Uniform Buffers",
                   resource_manager_.listUniformBufferNames(),
                   resource_manager_.uniformBufferCount());

    renderCategory("Textures", resource_manager_.listTextureImageNames(),
                   resource_manager_.textureImageCount());

    renderCategory("Cubemaps", resource_manager_.listCubemapNames(),
                   resource_manager_.cubemapCount());
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

  if (selected_type_ == "Meshes") {
    auto mesh = resource_manager_.getMesh(selected_name_);
    if (!mesh || !mesh->isValid()) {
      ImGui::TextDisabled("State: unavailable");
      return;
    }

    const auto vertexBuffer = mesh->vertexBufferBase();
    const auto indexBuffer = mesh->indexBuffer();
    if (!vertexBuffer || !indexBuffer) {
      ImGui::TextDisabled("State: unavailable");
      return;
    }

    const auto vertexInput = vertexBuffer->get().vertexInputDesc();

    ImGui::Text("State: valid");
    ImGui::Text("Vertices: %zu", vertexBuffer->get().vertexCount());
    ImGui::Text("Indices: %zu", indexBuffer->get().indices().size());
    ImGui::Text("Vertex bindings: %zu", vertexInput.bindings.size());
    ImGui::Text("Vertex attributes: %zu", vertexInput.attributes.size());
    return;
  }

  if (selected_type_ == "Uniform Buffers") {
    auto uniformBuffer = resource_manager_.getUniformBuffer(selected_name_);
    if (!uniformBuffer) {
      ImGui::TextDisabled("State: unavailable");
      return;
    }

    ImGui::Text("State: valid");
    ImGui::Text("Buffer size: %llu bytes",
                static_cast<unsigned long long>(uniformBuffer->bufferSize()));
    ImGui::Text("Frames: %zu", uniformBuffer->targetCount());
    ImGui::Text("Mapped frames: %zu", uniformBuffer->mappedCount());
    return;
  }

  if (selected_type_ == "Textures") {
    auto texture = resource_manager_.getTexture(selected_name_);
    if (!texture) {
      ImGui::TextDisabled("State: unavailable");
      return;
    }

    const auto &desc = texture->desc();

    ImGui::Text("State: %s", texture->valid() ? "valid" : "invalid");
    ImGui::Text("Size: %ux%u", texture->width(), texture->height());
    ImGui::Text("Format: %d", static_cast<int>(desc.image.format));
    ImGui::Text("Layout: %d", static_cast<int>(texture->layout()));
    ImGui::Text("Image: %s", texture->hasImage() ? "yes" : "no");
    ImGui::Text("View: %s", texture->hasImageView() ? "yes" : "no");
    ImGui::Text("Sampler: %s", texture->hasSampler() ? "yes" : "no");
    return;
  }

  if (selected_type_ == "Cubemaps") {
    auto cubemap = resource_manager_.getCubemap(selected_name_);
    if (!cubemap) {
      ImGui::TextDisabled("State: unavailable");
      return;
    }

    const auto &desc = cubemap->desc();

    ImGui::Text("State: %s", cubemap->valid() ? "valid" : "invalid");
    ImGui::Text("Size: %ux%u", cubemap->width(), cubemap->height());
    ImGui::Text("Format: %d", static_cast<int>(desc.format));
    ImGui::Text("Layout: %d", static_cast<int>(cubemap->layout()));
  }
}

} // namespace vkr::ui
