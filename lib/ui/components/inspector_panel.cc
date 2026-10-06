#include "vkr/ui/components/inspector_panel.hh"
#include "property_table.hh"
#include "vkr/exec/render/targets/offscreen.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include <array>
#include <imgui.h>
#include <string>
#include <string_view>
#include <utility>
#include <vulkan/vulkan.hpp>

namespace vkr::ui {

InspectorPanel::InspectorPanel(
    const scene::Scene &scene, const exec::Graph &graph,
    const Selection &selection, std::function<void(Selection)> onSelect,
    std::function<void(const scene::Texture &)> renderTexture)
    : UiComponent("Inspector"), scene_(scene), graph_(graph),
      selection_(selection), on_select_(std::move(onSelect)),
      render_texture_(std::move(renderTexture)) {}

void InspectorPanel::render() {
  if (selection_.type == SelectionType::None) {
    inspected_pass_.reset();
    ImGui::TextDisabled("Select an item to inspect.");
    return;
  }

  if (ImGui::SmallButton("Clear selection")) {
    on_select_({});
    inspected_pass_.reset();
    return;
  }

  std::string_view type{};
  switch (selection_.type) {
  case SelectionType::Mesh:
    type = "Mesh";
    break;
  case SelectionType::UniformBuffer:
    type = "Uniform buffer";
    break;
  case SelectionType::Texture:
    type = "Texture";
    break;
  case SelectionType::Cubemap:
    type = "Cubemap";
    break;
  case SelectionType::Pass:
    type = "Pass";
    break;
  case SelectionType::None:
    break;
  }
  ImGui::PushID(static_cast<int>(selection_.type));
  ImGui::PushID(selection_.name.c_str());
  ImGui::SeparatorText(type.data());
  ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.15f);
  ImGui::TextWrapped("%s", selection_.name.c_str());
  ImGui::PopFont();
  if (ImGui::BeginPopupContextItem("##selection_actions")) {
    if (ImGui::MenuItem("Copy name")) {
      ImGui::SetClipboardText(selection_.name.c_str());
    }
    ImGui::EndPopup();
  }
  ImGui::PushTextWrapPos();
  ImGui::TextDisabled("Right-click a name or value to copy.");
  ImGui::PopTextWrapPos();
  ImGui::Spacing();
  if (selection_.type == SelectionType::Pass) {
    renderPass();
  } else {
    inspected_pass_.reset();
    renderResource();
  }
  ImGui::PopID();
  ImGui::PopID();
}

void InspectorPanel::renderResource() {
  if (selection_.type == SelectionType::Mesh) {
    const auto mesh = scene_.findMesh(selection_.name);
    if (!mesh || !mesh->get().isValid()) {
      ImGui::TextDisabled("State: unavailable");
      return;
    }

    const auto vertexBuffer = mesh->get().vertexBufferBase();
    const auto indexBuffer = mesh->get().indexBuffer();
    if (!vertexBuffer || !indexBuffer) {
      ImGui::TextDisabled("State: unavailable");
      return;
    }

    if (beginPropertyTable("##mesh_summary")) {
      propertyRow("State", "Valid");
      propertyRow("Vertices", std::to_string(vertexBuffer->get().vertexCount()));
      propertyRow("Indices", std::to_string(indexBuffer->get().indices().size()));
      ImGui::EndTable();
    }
    if (ImGui::CollapsingHeader("Advanced")) {
      const auto vertexInput = vertexBuffer->get().vertexInputDesc();
      if (beginPropertyTable("##mesh_advanced")) {
        propertyRow("Bindings", std::to_string(vertexInput.bindings.size()));
        propertyRow("Attributes", std::to_string(vertexInput.attributes.size()));
        ImGui::EndTable();
      }
    }
    return;
  }

  if (selection_.type == SelectionType::UniformBuffer) {
    const auto uniformBuffer = scene_.findUniformBuffer(selection_.name);
    if (!uniformBuffer) {
      ImGui::TextDisabled("State: unavailable");
      return;
    }

    const auto &value = uniformBuffer->get();
    if (beginPropertyTable("##buffer_summary")) {
      propertyRow("State", "Valid");
      propertyRow("Buffer size", std::to_string(value.byteSize()) + " bytes");
      propertyRow("Frames", std::to_string(value.frameCount()));
      ImGui::EndTable();
    }
    if (ImGui::CollapsingHeader("Advanced") &&
        beginPropertyTable("##buffer_advanced")) {
      propertyRow("Mapped frames", std::to_string(value.mappedFrameCount()));
      ImGui::EndTable();
    }
    return;
  }

  if (selection_.type == SelectionType::Texture) {
    const auto texture = scene_.findTexture(selection_.name);
    if (!texture) {
      ImGui::TextDisabled("State: unavailable");
      return;
    }

    const auto &value = texture->get();
    const auto &desc = value.desc();
    const std::string_view source = desc.filePath;
    if (beginPropertyTable("##texture_summary")) {
      propertyRow("State", value.valid() ? "Valid" : "Invalid");
      propertyRow("Size", std::to_string(value.width()) + " x " +
                              std::to_string(value.height()));
      propertyRow("Format",
                  vk::to_string(static_cast<vk::Format>(desc.image.format)));
      propertyRow("Source", source.empty() ? "No file" : source);
      ImGui::EndTable();
    }
    if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen)) {
      render_texture_(value);
    }
    if (ImGui::CollapsingHeader("Advanced") &&
        beginPropertyTable("##texture_advanced")) {
      propertyRow("Layout",
                  vk::to_string(static_cast<vk::ImageLayout>(value.layout())));
      propertyRow("Image", value.hasImage() ? "Available" : "Missing");
      propertyRow("View", value.hasImageView() ? "Available" : "Missing");
      propertyRow("Sampler", value.hasSampler() ? "Available" : "None");
      ImGui::EndTable();
    }
    return;
  }

  if (selection_.type == SelectionType::Cubemap) {
    const auto cubemap = scene_.findCubemap(selection_.name);
    if (!cubemap) {
      ImGui::TextDisabled("State: unavailable");
      return;
    }

    const auto &value = cubemap->get();
    const auto &desc = value.desc();
    if (beginPropertyTable("##cubemap_summary")) {
      propertyRow("State", value.valid() ? "Valid" : "Invalid");
      propertyRow("Face size", std::to_string(value.width()) + " x " +
                                   std::to_string(value.height()));
      propertyRow("Format", vk::to_string(static_cast<vk::Format>(desc.format)));
      ImGui::EndTable();
    }
    if (ImGui::CollapsingHeader("Source files") &&
        beginPropertyTable("##cubemap_sources")) {
      constexpr std::array<std::string_view, 6> faces{
          "+X", "-X", "+Y", "-Y", "+Z", "-Z"};
      for (size_t index = 0; index < faces.size(); ++index) {
        const std::string_view path = desc.facePaths[index];
        propertyRow(faces[index], path.empty() ? "None" : path);
      }
      ImGui::EndTable();
    }
    if (ImGui::CollapsingHeader("Advanced") &&
        beginPropertyTable("##cubemap_advanced")) {
      propertyRow("Layout",
                  vk::to_string(static_cast<vk::ImageLayout>(value.layout())));
      ImGui::EndTable();
    }
  }
}

void InspectorPanel::renderPass() {
  if (!inspected_pass_ ||
      inspected_pass_->pass.get().name() != selection_.name) {
    const auto pass = graph_.getPass<exec::Pass>(selection_.name);
    if (!pass) {
      inspected_pass_.reset();
      ImGui::TextDisabled("State: unavailable");
      return;
    }
    inspected_pass_ = PassEntry{
        *pass,
        pass->get().capability<exec::GraphicsPipelineCapability>(),
        pass->get().capability<exec::RenderTargetCapability>(),
        pass->get().capability<exec::PresentCapability>().has_value(),
        graph_.dependencies(pass->get()),
    };
  }
  const auto &entry = *inspected_pass_;
  const auto &pass = entry.pass.get();
  const auto pipeline = entry.pipeline
                            ? entry.pipeline->get().editablePipeline()
                            : std::nullopt;

  if (beginPropertyTable("##pass_summary")) {
    if (pipeline) {
      const std::string_view name = pipeline->get().desc().name;
      propertyRow("Pipeline", name.empty() ? "<unnamed>" : name);
    } else {
      propertyRow("Pipeline", entry.pipeline ? "Unavailable" : "None");
    }
    propertyRow("Presentation", entry.presents ? "After submit" : "No");
    ImGui::EndTable();
  }

  if (entry.target) {
    const auto &target = entry.target->get().target(0);
    const auto &desc = target.desc();
    ImGui::SeparatorText("Render target");
    if (beginPropertyTable("##target_summary")) {
      propertyRow("Extent", std::to_string(target.width()) + " x " +
                                std::to_string(target.height()));
      propertyRow("Color format",
                  target.hasColor()
                      ? vk::to_string(static_cast<vk::Format>(desc.color.format))
                      : "None");
      propertyRow("Depth format",
                  desc.depth
                      ? vk::to_string(static_cast<vk::Format>(desc.depth->format))
                      : "None");
      ImGui::EndTable();
    }
    ImGui::PushTextWrapPos();
    ImGui::TextDisabled("Attachment configuration, frame 0");
    ImGui::PopTextWrapPos();
  }

  if (ImGui::CollapsingHeader("Dependencies")) {
    if (entry.dependencies.empty()) {
      ImGui::TextDisabled("None");
    } else if (beginPropertyTable("##pass_dependencies")) {
      for (size_t index = 0; index < entry.dependencies.size(); ++index) {
        propertyRow("Producer " + std::to_string(index + 1),
                    entry.dependencies[index].get().name());
      }
      ImGui::EndTable();
    }
  }

  if (ImGui::CollapsingHeader("Advanced")) {
    ImGui::SeparatorText("Capabilities");
    if (beginPropertyTable("##pass_capabilities")) {
      propertyRow("Graphics pipeline", entry.pipeline ? "Yes" : "No");
      propertyRow("Render target", entry.target ? "Yes" : "No");
      propertyRow("Presentation", entry.presents ? "Yes" : "No");
      ImGui::EndTable();
    }
    if (entry.target) {
      const auto &target = entry.target->get().target(0);
      const auto &desc = target.desc();
      ImGui::SeparatorText("Attachments");
      if (beginPropertyTable("##target_advanced")) {
        if (target.hasColor()) {
          const bool sampled =
              (desc.color.usage & VK_IMAGE_USAGE_SAMPLED_BIT) != 0;
          propertyRow("Color sampled", sampled ? "Yes" : "No");
        }
        if (desc.depth) {
          const bool retained =
              desc.depth->storeOp == VK_ATTACHMENT_STORE_OP_STORE ||
              (desc.depth->usage & VK_IMAGE_USAGE_SAMPLED_BIT) != 0;
          propertyRow("Depth retained", retained ? "Yes" : "No");
        }
        ImGui::EndTable();
      }
    }
    ImGui::SeparatorText("Declared reads");
    if (pass.reads().empty()) {
      ImGui::TextDisabled("None");
    } else if (beginPropertyTable("##pass_reads")) {
      for (size_t index = 0; index < pass.reads().size(); ++index) {
        propertyRow("Read " + std::to_string(index + 1), pass.reads()[index]);
      }
      ImGui::EndTable();
    }
    ImGui::SeparatorText("Declared writes");
    if (pass.writes().empty()) {
      ImGui::TextDisabled("None");
    } else if (beginPropertyTable("##pass_writes")) {
      for (size_t index = 0; index < pass.writes().size(); ++index) {
        propertyRow("Write " + std::to_string(index + 1), pass.writes()[index]);
      }
      ImGui::EndTable();
    }
  }
}

} // namespace vkr::ui
