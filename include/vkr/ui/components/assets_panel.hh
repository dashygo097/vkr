#pragma once

#include "vkr/ui/components/ui_component.hh"
#include "vkr/util/asset.hh"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace vkr::ui {

class AssetsPanel final : public UiComponent {
public:
  explicit AssetsPanel(const util::AssetSystem &assetSystem);

private:
  void render() override;
  void refresh();

  const util::AssetSystem &asset_system_;
  int selected_root_{0};
  std::filesystem::path root_{};
  std::vector<std::string> files_{};
  std::vector<size_t> filtered_files_{};
  std::optional<size_t> selected_file_{};
  ImGuiTextFilter filter_{};
  std::string scan_error_{};
  bool truncated_{false};
};

} // namespace vkr::ui
