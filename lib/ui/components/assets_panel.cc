#include "vkr/ui/components/assets_panel.hh"
#include <algorithm>
#include <filesystem>
#include <imgui.h>
#include <vector>

namespace vkr::ui {

AssetsPanel::AssetsPanel(const util::AssetSystem &assetSystem)
    : UiComponent("Assets"), asset_system_(assetSystem) {}

void AssetsPanel::render() {
  const auto normalizeRoot = [](const std::string &root) {
    auto path = std::filesystem::path(root);
    if (path.is_relative()) {
      path = std::filesystem::absolute(path);
    }

    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(path, ec);
    return ec ? path.lexically_normal() : canonical.lexically_normal();
  };

  const auto engineRoot = normalizeRoot(asset_system_.desc().engineRoot);
  const auto appRoot = normalizeRoot(asset_system_.desc().appRoot);
  const auto userRoot = normalizeRoot(asset_system_.desc().userRoot);

  const auto isInsideRoot = [](const std::filesystem::path &path,
                               const std::filesystem::path &root) {
    std::error_code ec;
    auto relative = std::filesystem::relative(path, root, ec);
    if (ec || relative.empty()) {
      return false;
    }

    for (const auto &part : relative) {
      if (part == "..") {
        return false;
      }
    }

    return true;
  };

  auto renderRoot = [&](const char *label, const std::filesystem::path &root,
                        bool hidden, bool defaultOpen) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;

    if (defaultOpen) {
      flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (!ImGui::TreeNodeEx(label, flags, "%s", label)) {
      return;
    }

    ImGui::TextDisabled("%s", root.string().c_str());

    if (hidden) {
      ImGui::TextDisabled("Hidden because this root is engine defaults.");
      ImGui::TreePop();
      return;
    }

    if (!std::filesystem::exists(root)) {
      ImGui::TextDisabled("Not found");
      ImGui::TreePop();
      return;
    }

    std::vector<std::filesystem::path> entries{};
    std::error_code ec;
    std::filesystem::recursive_directory_iterator iterator(
        root, std::filesystem::directory_options::skip_permission_denied, ec);
    std::filesystem::recursive_directory_iterator end{};

    size_t skippedEngineDefaults = 0;
    size_t skippedDirectories = 0;
    size_t visibleLimit = 300;
    bool truncated = false;

    for (; iterator != end && !ec; iterator.increment(ec)) {
      if (ec) {
        break;
      }

      const auto path = iterator->path();
      const auto filename = path.filename().string();

      if (iterator->is_directory(ec)) {
        if (filename == ".git" || filename == "build" ||
            filename == "3rdparty" || filename == ".cache") {
          iterator.disable_recursion_pending();
          skippedDirectories++;
        }
        continue;
      }

      if (isInsideRoot(path, engineRoot)) {
        skippedEngineDefaults++;
        continue;
      }

      if (!iterator->is_regular_file(ec)) {
        continue;
      }

      if (entries.size() >= visibleLimit) {
        truncated = true;
        continue;
      }

      entries.push_back(path);
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

    if (skippedEngineDefaults > 0 || skippedDirectories > 0) {
      ImGui::TextDisabled("Hidden: %zu engine files, %zu large folders",
                          skippedEngineDefaults, skippedDirectories);
    }

    ImGui::TreePop();
  };

  ImGui::SeparatorText("Target Roots");
  ImGui::Checkbox("App", &show_app_assets_);
  ImGui::SameLine();
  ImGui::Checkbox("User", &show_user_assets_);

  ImGui::SeparatorText("Project Assets");
  if (show_app_assets_) {
    renderRoot("App", appRoot, appRoot == engineRoot, true);
  }

  if (show_user_assets_) {
    renderRoot("User", userRoot, userRoot == engineRoot, false);
  }

  if (!show_app_assets_ && !show_user_assets_) {
    ImGui::TextDisabled("No asset root selected");
  }

  ImGui::Spacing();
  ImGui::SeparatorText("Engine Defaults");
  ImGui::TextDisabled("Engine default assets are hidden from this browser.");
}

} // namespace vkr::ui
