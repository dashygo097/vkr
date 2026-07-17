#include "vkr/ui/components/assets_panel.hh"
#include <algorithm>
#include <filesystem>
#include <imgui.h>
#include <system_error>
#include <vector>

namespace vkr::ui {

namespace {

auto normalizeRoot(const std::string &root) -> std::filesystem::path {
  auto path = std::filesystem::path(root);
  if (path.is_relative()) {
    std::error_code absoluteEc;
    auto absolute = std::filesystem::absolute(path, absoluteEc);
    if (!absoluteEc) {
      path = absolute;
    }
  }

  std::error_code canonicalEc;
  auto canonical = std::filesystem::weakly_canonical(path, canonicalEc);
  return canonicalEc ? path.lexically_normal() : canonical.lexically_normal();
}

} // namespace

AssetsPanel::AssetsPanel(const util::AssetSystem &assetSystem)
    : UiComponent("Assets"), asset_system_(assetSystem) {}

void AssetsPanel::render() {
  const auto appRoot = normalizeRoot(asset_system_.desc().appRoot);
  const auto userRoot = normalizeRoot(asset_system_.desc().userRoot);

  auto renderRoot = [&](const char *label, const std::filesystem::path &root,
                        bool defaultOpen, bool hideAssetsDirectory) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;

    if (defaultOpen) {
      flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (!ImGui::TreeNodeEx(label, flags, "%s", label)) {
      return;
    }

    ImGui::TextDisabled("%s", root.string().c_str());

    std::error_code existsEc;
    if (!std::filesystem::exists(root, existsEc)) {
      if (existsEc) {
        ImGui::TextDisabled("Cannot inspect: %s", existsEc.message().c_str());
      }
      ImGui::TextDisabled("Not found");
      ImGui::TreePop();
      return;
    }

    std::error_code directoryEc;
    if (!std::filesystem::is_directory(root, directoryEc)) {
      if (directoryEc) {
        ImGui::TextDisabled("Cannot inspect: %s",
                            directoryEc.message().c_str());
      } else {
        ImGui::TextDisabled("Not a directory");
      }
      ImGui::TreePop();
      return;
    }

    std::vector<std::filesystem::path> entries{};
    std::error_code iteratorEc;
    std::filesystem::recursive_directory_iterator iterator(
        root, std::filesystem::directory_options::skip_permission_denied,
        iteratorEc);
    std::filesystem::recursive_directory_iterator end{};

    size_t skippedDirectories = 0;
    size_t skippedEntries = 0;
    size_t visibleLimit = 300;
    bool truncated = false;

    if (iteratorEc) {
      ImGui::TextDisabled("Cannot open: %s", iteratorEc.message().c_str());
      ImGui::TreePop();
      return;
    }

    for (; iterator != end;) {
      const auto path = iterator->path();
      const auto filename = path.filename().string();

      std::error_code entryEc;
      if (iterator->is_directory(entryEc)) {
        const bool isHiddenAssetsDirectory =
            hideAssetsDirectory && iterator.depth() == 0 &&
            filename == "assets";
        if (filename == ".git" || filename == "build" ||
            filename == "3rdparty" || filename == ".cache") {
          iterator.disable_recursion_pending();
          skippedDirectories++;
        } else if (isHiddenAssetsDirectory) {
          iterator.disable_recursion_pending();
          skippedDirectories++;
        }
      } else if (entryEc) {
        skippedEntries++;
      } else {
        std::error_code regularEc;
        if (iterator->is_regular_file(regularEc)) {
          if (entries.size() >= visibleLimit) {
            truncated = true;
          } else {
            entries.push_back(path);
          }
        } else if (regularEc) {
          skippedEntries++;
        }
      }

      std::error_code incrementEc;
      iterator.increment(incrementEc);
      if (incrementEc) {
        skippedEntries++;
        iteratorEc = incrementEc;
        break;
      }
    }

    std::sort(entries.begin(), entries.end());

    if (entries.empty()) {
      ImGui::TextDisabled("No files");
    } else {
      for (const auto &path : entries) {
        std::error_code relEc;
        auto rel = std::filesystem::relative(path, root, relEc);
        ImGui::BulletText("%s",
                          (relEc ? path.filename() : rel).string().c_str());
      }
    }

    if (truncated) {
      ImGui::TextDisabled("Showing first %zu files", visibleLimit);
    }

    if (skippedDirectories > 0) {
      ImGui::TextDisabled("Hidden: %zu large folders", skippedDirectories);
    }

    if (skippedEntries > 0) {
      ImGui::TextDisabled("Skipped: %zu unreadable entries", skippedEntries);
    }

    if (iteratorEc) {
      ImGui::TextDisabled("Traversal stopped: %s",
                          iteratorEc.message().c_str());
    }

    ImGui::TreePop();
  };

  ImGui::SeparatorText("Target Roots");
  ImGui::Checkbox("App##assets_show_app", &show_app_assets_);
  ImGui::SameLine();
  ImGui::Checkbox("User##assets_show_user", &show_user_assets_);

  ImGui::SeparatorText("Project Assets");
  if (show_app_assets_) {
    renderRoot("App", appRoot, true, false);
  }

  if (show_user_assets_) {
    renderRoot("User", userRoot, false, true);
  }

  if (!show_app_assets_ && !show_user_assets_) {
    ImGui::TextDisabled("No asset root selected");
  }
}

} // namespace vkr::ui
