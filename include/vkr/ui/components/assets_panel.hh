#pragma once

#include "vkr/ui/components/ui_component.hh"
#include "vkr/util/asset.hh"

namespace vkr::ui {

class AssetsPanel final : public UiComponent {
public:
  explicit AssetsPanel(const util::AssetSystem &assetSystem);

private:
  void render() override;

  const util::AssetSystem &asset_system_;
  bool show_app_assets_{true};
  bool show_user_assets_{false};
};

} // namespace vkr::ui
