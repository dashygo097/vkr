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
    : UiComponent("Assets"), asset_system_(assetSystem) {
  refresh();
}

void AssetsPanel::refresh() {
  root_ = normalizeRoot(selected_root_ == 0 ? asset_system_.desc().appRoot
                                            : asset_system_.desc().userRoot);
  files_.clear();
  filtered_files_.clear();
  selected_file_.reset();
  scan_error_.clear();
  truncated_ = false;

  std::error_code error;
  std::filesystem::recursive_directory_iterator iterator(
      root_, std::filesystem::directory_options::skip_permission_denied, error);
  const std::filesystem::recursive_directory_iterator end{};
  if (error) {
    scan_error_ = error.message();
    return;
  }

  size_t visited = 0;
  for (; iterator != end; iterator.increment(error)) {
    if (error) {
      break;
    }
    if (++visited > 10000 || files_.size() >= 4096) {
      truncated_ = true;
      break;
    }

    const auto &entry = *iterator;
    const auto filename = entry.path().filename().string();
    std::error_code entryError;
    if (entry.is_directory(entryError)) {
      if (filename == ".git" || filename == "build" || filename == "3rdparty" ||
          filename == ".cache" ||
          (selected_root_ == 1 && iterator.depth() == 0 &&
           filename == "assets")) {
        iterator.disable_recursion_pending();
      }
    } else if (!entryError && entry.is_regular_file(entryError) &&
               !entryError) {
      files_.push_back(entry.path().lexically_relative(root_).generic_string());
    }
  }
  if (error) {
    scan_error_ = error.message();
  }

  std::sort(files_.begin(), files_.end());
  for (size_t index = 0; index < files_.size(); ++index) {
    if (filter_.PassFilter(files_[index].c_str())) {
      filtered_files_.push_back(index);
    }
  }
}

void AssetsPanel::render() {
  const float refreshWidth = ImGui::CalcTextSize("Refresh").x +
                             ImGui::GetStyle().FramePadding.x * 2.0f;
  ImGui::SetNextItemWidth(std::max(1.0f, ImGui::GetContentRegionAvail().x -
                                             refreshWidth -
                                             ImGui::GetStyle().ItemSpacing.x));
  bool refreshRequested = ImGui::Combo("##asset_root", &selected_root_,
                                       "Application assets\0Project files\0");
  ImGui::SameLine();
  refreshRequested |= ImGui::Button("Refresh");
  if (refreshRequested) {
    refresh();
  }

  ImGui::SetNextItemWidth(-1.0f);
  if (ImGui::InputTextWithHint("##asset_filter", "Filter files...",
                               filter_.InputBuf, sizeof(filter_.InputBuf))) {
    filter_.Build();
    filtered_files_.clear();
    for (size_t index = 0; index < files_.size(); ++index) {
      if (filter_.PassFilter(files_[index].c_str())) {
        filtered_files_.push_back(index);
      }
    }
  }

  ImGui::TextDisabled("%zu files%s", filtered_files_.size(),
                      truncated_ ? " (limited)" : "");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", root_.string().c_str());
  }

  if (!scan_error_.empty()) {
    ImGui::TextWrapped("Cannot fully inspect this root: %s",
                       scan_error_.c_str());
  }
  if (filtered_files_.empty()) {
    ImGui::TextDisabled(files_.empty() ? "No files in this root"
                                       : "No matching files");
    return;
  }

  const ImGuiTableFlags flags =
      ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
      ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV;
  if (ImGui::BeginTable(
          "##asset_files", 2, flags,
          ImVec2(0.0f, std::max(1.0f, ImGui::GetContentRegionAvail().y)))) {
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.6f);
    ImGui::TableSetupColumn("Folder", ImGuiTableColumnFlags_WidthStretch, 0.4f);
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(filtered_files_.size()));
    while (clipper.Step()) {
      for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
        const size_t index = filtered_files_[static_cast<size_t>(row)];
        const auto &path = files_[index];
        const size_t slash = path.find_last_of('/');
        const size_t nameStart = slash == std::string::npos ? 0 : slash + 1;

        ImGui::PushID(static_cast<int>(index));
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        if (ImGui::Selectable(path.c_str() + nameStart, selected_file_ == index,
                              ImGuiSelectableFlags_SpanAllColumns)) {
          selected_file_ = index;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", path.c_str());
        }
        if (ImGui::BeginPopupContextItem("##asset_actions")) {
          if (ImGui::MenuItem("Copy relative path")) {
            ImGui::SetClipboardText(path.c_str());
          }
          if (ImGui::MenuItem("Copy full path")) {
            ImGui::SetClipboardText((root_ / path).string().c_str());
          }
          ImGui::EndPopup();
        }

        ImGui::TableSetColumnIndex(1);
        if (slash == std::string::npos) {
          ImGui::TextDisabled("-");
        } else {
          ImGui::TextUnformatted(path.data(), path.data() + slash);
        }
        ImGui::PopID();
      }
    }
    ImGui::EndTable();
  }
}

} // namespace vkr::ui
