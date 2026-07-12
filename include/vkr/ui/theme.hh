#pragma once

#include <cstdint>
#include <imgui.h>

namespace vkr::ui {

enum class ThemeAccent : uint8_t {
  Blue = 0,
  Red = 1,
  Green = 2,
  Purple = 3,
  Amber = 4,
};

struct ThemeDesc {
  ThemeAccent accent{ThemeAccent::Blue};
  float rounding{4.0f};
  float alpha{1.0f};
  bool dark{true};

  [[nodiscard]] auto isValid() const noexcept -> bool { return true; }

  template <typename Archive> auto serialize(Archive &ar) -> void {
    ar("accent", accent);
    ar("alpha", alpha);
    ar("dark", dark);
    ar("rounding", rounding);
  }
};

class Theme {
public:
  static void apply(const ThemeDesc &config);

  static auto accentColor(ThemeAccent accent) noexcept -> ImVec4;
  static auto accentHoverColor(ThemeAccent accent) noexcept -> ImVec4;
  static auto accentActiveColor(ThemeAccent accent) noexcept -> ImVec4;

private:
  static void applyDarkBase(ImGuiStyle &style);
};

} // namespace vkr::ui
