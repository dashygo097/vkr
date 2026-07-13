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

void Theme::applyLightBase(ImGuiStyle &style) {
  ImVec4 *colors = style.Colors;

  colors[ImGuiCol_Text] = ImVec4(0.09f, 0.11f, 0.15f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.39f, 0.43f, 0.50f, 1.00f);

  colors[ImGuiCol_WindowBg] = ImVec4(0.88f, 0.90f, 0.93f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.94f, 0.95f, 0.97f, 0.98f);

  colors[ImGuiCol_Border] = ImVec4(0.68f, 0.72f, 0.78f, 1.00f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  colors[ImGuiCol_FrameBg] = ImVec4(0.80f, 0.84f, 0.89f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.75f, 0.80f, 0.86f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.70f, 0.76f, 0.84f, 1.00f);

  colors[ImGuiCol_TitleBg] = ImVec4(0.80f, 0.84f, 0.89f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.74f, 0.79f, 0.86f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.83f, 0.86f, 0.90f, 1.00f);

  colors[ImGuiCol_MenuBarBg] = ImVec4(0.82f, 0.85f, 0.90f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.84f, 0.87f, 0.91f, 1.00f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.59f, 0.64f, 0.72f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.50f, 0.56f, 0.66f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.42f, 0.49f, 0.60f, 1.00f);

  colors[ImGuiCol_Header] = ImVec4(0.78f, 0.82f, 0.88f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.76f, 0.84f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.64f, 0.71f, 0.81f, 1.00f);

  colors[ImGuiCol_Tab] = ImVec4(0.78f, 0.82f, 0.88f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.68f, 0.76f, 0.87f, 1.00f);
  colors[ImGuiCol_TabActive] = ImVec4(0.91f, 0.93f, 0.96f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.82f, 0.85f, 0.89f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.88f, 0.91f, 0.94f, 1.00f);

  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.76f, 0.81f, 0.88f, 1.00f);
  colors[ImGuiCol_TableBorderStrong] = ImVec4(0.60f, 0.66f, 0.75f, 1.00f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.70f, 0.75f, 0.82f, 1.00f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.54f, 0.62f, 0.74f, 0.14f);

  colors[ImGuiCol_DockingPreview] = ImVec4(0.20f, 0.45f, 0.90f, 0.35f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.84f, 0.87f, 0.91f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.63f, 0.68f, 0.76f, 1.00f);
  colors[ImGuiCol_InputTextCursor] = ImVec4(0.08f, 0.11f, 0.16f, 1.00f);
  colors[ImGuiCol_TextLink] = ImVec4(0.10f, 0.31f, 0.72f, 1.00f);
  colors[ImGuiCol_TreeLines] = ImVec4(0.56f, 0.62f, 0.70f, 1.00f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.24f, 0.31f, 0.25f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.24f, 0.31f, 0.35f);
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
  } else {
    applyLightBase(style);
  }

  const ImVec4 accent = accentColor(config.accent);
  const ImVec4 accentHover = accentHoverColor(config.accent);
  const ImVec4 accentActive = accentActiveColor(config.accent);
  const float buttonAlpha = config.dark ? 0.55f : 0.22f;
  const float buttonHoverAlpha = config.dark ? 0.75f : 0.34f;
  const float buttonActiveAlpha = config.dark ? 0.90f : 0.46f;
  const float headerHoverAlpha = config.dark ? 0.35f : 0.18f;
  const float headerActiveAlpha = config.dark ? 0.50f : 0.28f;
  const float tabHoverAlpha = config.dark ? 0.60f : 0.24f;
  const float tabActiveAlpha = config.dark ? 0.35f : 0.16f;
  const float selectionAlpha = config.dark ? 0.35f : 0.24f;

  ImVec4 *colors = style.Colors;

  colors[ImGuiCol_CheckMark] = accent;
  colors[ImGuiCol_SliderGrab] = accent;
  colors[ImGuiCol_SliderGrabActive] = accentActive;

  colors[ImGuiCol_Button] = ImVec4(accent.x, accent.y, accent.z, buttonAlpha);
  colors[ImGuiCol_ButtonHovered] =
      ImVec4(accentHover.x, accentHover.y, accentHover.z, buttonHoverAlpha);
  colors[ImGuiCol_ButtonActive] =
      ImVec4(accentActive.x, accentActive.y, accentActive.z, buttonActiveAlpha);

  colors[ImGuiCol_SeparatorHovered] = accentHover;
  colors[ImGuiCol_SeparatorActive] = accentActive;

  colors[ImGuiCol_ResizeGrip] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
  colors[ImGuiCol_ResizeGripHovered] =
      ImVec4(accentHover.x, accentHover.y, accentHover.z, 0.60f);
  colors[ImGuiCol_ResizeGripActive] =
      ImVec4(accentActive.x, accentActive.y, accentActive.z, 0.90f);

  colors[ImGuiCol_HeaderHovered] =
      ImVec4(accent.x, accent.y, accent.z, headerHoverAlpha);
  colors[ImGuiCol_HeaderActive] =
      ImVec4(accent.x, accent.y, accent.z, headerActiveAlpha);

  colors[ImGuiCol_TabHovered] =
      ImVec4(accentHover.x, accentHover.y, accentHover.z, tabHoverAlpha);
  colors[ImGuiCol_TabActive] =
      ImVec4(accent.x, accent.y, accent.z, tabActiveAlpha);
  colors[ImGuiCol_TabSelectedOverline] =
      ImVec4(accent.x, accent.y, accent.z, config.dark ? 0.80f : 0.65f);
  colors[ImGuiCol_TabDimmedSelectedOverline] =
      ImVec4(accent.x, accent.y, accent.z, config.dark ? 0.45f : 0.38f);

  colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.45f);
  colors[ImGuiCol_TextSelectedBg] =
      ImVec4(accent.x, accent.y, accent.z, selectionAlpha);
  colors[ImGuiCol_NavHighlight] = accent;

  style.Alpha = config.alpha;
  style.WindowRounding = config.rounding;
  style.ChildRounding = config.rounding;
  style.FrameRounding = config.rounding;
  style.PopupRounding = config.rounding;
  style.ScrollbarRounding = config.rounding;
  style.GrabRounding = config.rounding;
  style.TabRounding = config.rounding;

  style.WindowPadding = ImVec2(12.0f, 10.0f);
  style.FramePadding = ImVec2(8.0f, 5.0f);
  style.CellPadding = ImVec2(8.0f, 5.0f);
  style.ItemSpacing = ImVec2(8.0f, 7.0f);
  style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
  style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
  style.IndentSpacing = 18.0f;
  style.ScrollbarSize = 12.0f;
  style.GrabMinSize = 10.0f;
  style.TabBarBorderSize = 1.0f;
  style.DockingSeparatorSize = 2.0f;
  style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
  style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
  style.SelectableTextAlign = ImVec2(0.0f, 0.5f);
  style.SeparatorTextBorderSize = 1.0f;
  style.SeparatorTextPadding = ImVec2(8.0f, 5.0f);

  style.WindowBorderSize = 1.0f;
  style.ChildBorderSize = 1.0f;
  style.FrameBorderSize = config.dark ? 0.0f : 1.0f;
  style.PopupBorderSize = 1.0f;
  style.TabBorderSize = 0.0f;
}

} // namespace vkr::ui
