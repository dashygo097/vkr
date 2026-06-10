#pragma once

#include <imgui.h>

namespace vkr::ui {

enum class ThemeAccent {
  Blue,
  Red,
  Green,
  Purple,
  Amber,
};

struct ThemeConfig {
  ThemeAccent accent{ThemeAccent::Blue};
  float rounding{4.0f};
  float alpha{1.0f};
  bool dark{true};
};

class Theme {
public:
  static void apply(const ThemeConfig &config);

  static auto accentColor(ThemeAccent accent) noexcept -> ImVec4;
  static auto accentHoverColor(ThemeAccent accent) noexcept -> ImVec4;
  static auto accentActiveColor(ThemeAccent accent) noexcept -> ImVec4;

private:
  static void applyDarkBase(ImGuiStyle &style);
};

} // namespace vkr::ui
