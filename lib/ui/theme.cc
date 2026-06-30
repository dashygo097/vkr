#include "vkr/ui/theme.hh"

namespace vkr::ui {

auto Theme::accentColor(ThemeAccent accent) noexcept -> ImVec4 {
  switch (accent) {
  case ThemeAccent::Blue:
    return ImVec4{0.25f, 0.55f, 1.00f, 1.00f};
  case ThemeAccent::Red:
    return ImVec4{0.95f, 0.25f, 0.25f, 1.00f};
  case ThemeAccent::Green:
    return ImVec4{0.25f, 0.80f, 0.45f, 1.00f};
  case ThemeAccent::Purple:
    return ImVec4{0.65f, 0.40f, 1.00f, 1.00f};
  case ThemeAccent::Amber:
    return ImVec4{1.00f, 0.62f, 0.20f, 1.00f};
  }

  return ImVec4{0.25f, 0.55f, 1.00f, 1.00f};
}

auto Theme::accentHoverColor(ThemeAccent accent) noexcept -> ImVec4 {
  ImVec4 color = accentColor(accent);
  color.x = color.x + (1.0f - color.x) * 0.15f;
  color.y = color.y + (1.0f - color.y) * 0.15f;
  color.z = color.z + (1.0f - color.z) * 0.15f;
  color.w = 1.0f;
  return color;
}

auto Theme::accentActiveColor(ThemeAccent accent) noexcept -> ImVec4 {
  ImVec4 color = accentColor(accent);
  color.x *= 0.80f;
  color.y *= 0.80f;
  color.z *= 0.80f;
  color.w = 1.0f;
  return color;
}

void Theme::applyDarkBase(ImGuiStyle &style) {
  ImVec4 *colors = style.Colors;

  colors[ImGuiCol_Text] = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.48f, 0.52f, 1.00f);

  colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.08f, 0.10f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.09f, 0.11f, 0.98f);

  colors[ImGuiCol_Border] = ImVec4(0.20f, 0.22f, 0.26f, 1.00f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);

  colors[ImGuiCol_TitleBg] = ImVec4(0.07f, 0.08f, 0.10f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.11f, 0.12f, 0.15f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.07f, 0.08f, 1.00f);

  colors[ImGuiCol_MenuBarBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.07f, 0.08f, 1.00f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.24f, 0.28f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.30f, 0.35f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.34f, 0.36f, 0.42f, 1.00f);

  colors[ImGuiCol_Header] = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.23f, 0.28f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.24f, 0.27f, 0.33f, 1.00f);

  colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.12f, 0.15f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.23f, 0.28f, 1.00f);
  colors[ImGuiCol_TabActive] = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.09f, 0.11f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);

  colors[ImGuiCol_DockingPreview] = ImVec4(0.25f, 0.55f, 1.00f, 0.45f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.05f, 0.06f, 0.07f, 1.00f);
}

void Theme::apply(const ThemeDesc &config) {
  if (config.dark) {
    ImGui::StyleColorsDark();
  } else {
    ImGui::StyleColorsLight();
  }

  ImGuiStyle &style = ImGui::GetStyle();

  if (config.dark) {
    applyDarkBase(style);
  }

  const ImVec4 accent = accentColor(config.accent);
  const ImVec4 accentHover = accentHoverColor(config.accent);
  const ImVec4 accentActive = accentActiveColor(config.accent);

  ImVec4 *colors = style.Colors;

  colors[ImGuiCol_CheckMark] = accent;
  colors[ImGuiCol_SliderGrab] = accent;
  colors[ImGuiCol_SliderGrabActive] = accentActive;

  colors[ImGuiCol_Button] = ImVec4(accent.x, accent.y, accent.z, 0.55f);
  colors[ImGuiCol_ButtonHovered] =
      ImVec4(accentHover.x, accentHover.y, accentHover.z, 0.75f);
  colors[ImGuiCol_ButtonActive] =
      ImVec4(accentActive.x, accentActive.y, accentActive.z, 0.90f);

  colors[ImGuiCol_SeparatorHovered] = accentHover;
  colors[ImGuiCol_SeparatorActive] = accentActive;

  colors[ImGuiCol_ResizeGrip] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
  colors[ImGuiCol_ResizeGripHovered] =
      ImVec4(accentHover.x, accentHover.y, accentHover.z, 0.60f);
  colors[ImGuiCol_ResizeGripActive] =
      ImVec4(accentActive.x, accentActive.y, accentActive.z, 0.90f);

  colors[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
  colors[ImGuiCol_HeaderActive] = ImVec4(accent.x, accent.y, accent.z, 0.50f);

  colors[ImGuiCol_TabHovered] =
      ImVec4(accentHover.x, accentHover.y, accentHover.z, 0.60f);
  colors[ImGuiCol_TabActive] = ImVec4(accent.x, accent.y, accent.z, 0.35f);

  colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.45f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
  colors[ImGuiCol_NavHighlight] = accent;

  style.Alpha = config.alpha;
  style.WindowRounding = config.rounding;
  style.ChildRounding = config.rounding;
  style.FrameRounding = config.rounding;
  style.PopupRounding = config.rounding;
  style.ScrollbarRounding = config.rounding;
  style.GrabRounding = config.rounding;
  style.TabRounding = config.rounding;

  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;
  style.PopupBorderSize = 1.0f;
  style.TabBorderSize = 0.0f;
}

} // namespace vkr::ui
