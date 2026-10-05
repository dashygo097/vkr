#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <imgui.h>
#include <utility>

namespace vkr::ui {

enum class ThemeAccent : uint8_t {
  Blue = 0,
  Red = 1,
  Green = 2,
  Purple = 3,
  Amber = 4,
};

enum class ThemeDensity : uint8_t {
  Compact,
  Comfortable,
};

struct ThemeDesc {
  ThemeAccent accent{ThemeAccent::Blue};
  float rounding{3.0f};
  float alpha{1.0f};
  bool dark{true};
  ThemeDensity density{ThemeDensity::Compact};
  float scale{1.0f};

  [[nodiscard]] auto isValid() const noexcept -> bool {
    return accent <= ThemeAccent::Amber &&
           (density == ThemeDensity::Compact ||
            density == ThemeDensity::Comfortable) &&
           std::isfinite(rounding) && rounding >= 0.0f && rounding <= 10.0f &&
           std::isfinite(alpha) && alpha >= 0.35f && alpha <= 1.0f &&
           std::isfinite(scale) && scale >= 0.75f && scale <= 2.0f;
  }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("accent", accent);
    ar("alpha", alpha);
    ar("dark", dark);
    ar("rounding", rounding);
    ar("density", density);
    ar("scale", scale);
  }
};

class Theme {
public:
  static void apply(const ThemeDesc &config, float dpiScale);

  // Interface and code fonts are owned by the current ImGui context.
  [[nodiscard]] static auto loadFonts()
      -> std::pair<std::reference_wrapper<ImFont>,
                   std::reference_wrapper<ImFont>>;

  static auto accentColor(ThemeAccent accent) noexcept -> ImVec4;
  static auto accentHoverColor(ThemeAccent accent) noexcept -> ImVec4;
  static auto accentActiveColor(ThemeAccent accent) noexcept -> ImVec4;

private:
  static void applyDarkBase(ImGuiStyle &style);
  static void applyLightBase(ImGuiStyle &style);
};

} // namespace vkr::ui
