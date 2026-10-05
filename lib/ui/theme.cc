#include "vkr/ui/theme.hh"
#include "vkr/logger.hh"
#include "vkr_ui_fonts.hh"
#include <cstdio>

namespace vkr::ui {

auto Theme::accentColor(ThemeAccent accent) noexcept -> ImVec4 {
  switch (accent) {
  case ThemeAccent::Blue:
    return ImVec4{0.32f, 0.60f, 0.94f, 1.00f};
  case ThemeAccent::Red:
    return ImVec4{0.95f, 0.25f, 0.25f, 1.00f};
  case ThemeAccent::Green:
    return ImVec4{0.25f, 0.80f, 0.45f, 1.00f};
  case ThemeAccent::Purple:
    return ImVec4{0.65f, 0.40f, 1.00f, 1.00f};
  case ThemeAccent::Amber:
    return ImVec4{1.00f, 0.62f, 0.20f, 1.00f};
  }

  return ImVec4{0.32f, 0.60f, 0.94f, 1.00f};
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
  auto &colors = style.Colors;

  colors[ImGuiCol_Text] = ImVec4(0.88f, 0.90f, 0.93f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.63f, 0.68f, 1.00f);

  colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.11f, 0.13f, 0.98f);

  colors[ImGuiCol_Border] = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.21f, 0.24f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.23f, 0.25f, 0.29f, 1.00f);

  colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.00f);
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
  colors[ImGuiCol_TabSelected] = ImVec4(0.19f, 0.21f, 0.24f, 1.00f);
  colors[ImGuiCol_TabDimmed] = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
  colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);

  colors[ImGuiCol_DockingPreview] = ImVec4(0.32f, 0.60f, 0.94f, 0.45f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.05f, 0.06f, 0.07f, 1.00f);
  colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
  colors[ImGuiCol_TableHeaderBg] = colors[ImGuiCol_FrameBg];
  colors[ImGuiCol_TableBorderStrong] = colors[ImGuiCol_Border];
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.16f, 0.18f, 0.21f, 1.0f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.025f);
  colors[ImGuiCol_TreeLines] = colors[ImGuiCol_Border];
}

void Theme::applyLightBase(ImGuiStyle &style) {
  auto &colors = style.Colors;

  colors[ImGuiCol_Text] = ImVec4(0.06f, 0.08f, 0.12f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.30f, 0.34f, 0.41f, 1.00f);

  colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.96f, 0.97f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.98f, 0.98f, 0.99f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.99f, 0.99f, 1.00f, 0.98f);

  colors[ImGuiCol_Border] = ImVec4(0.76f, 0.79f, 0.83f, 1.00f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  colors[ImGuiCol_FrameBg] = ImVec4(0.89f, 0.91f, 0.94f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.86f, 0.89f, 0.93f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.79f, 0.83f, 0.88f, 1.00f);

  colors[ImGuiCol_TitleBg] = ImVec4(0.91f, 0.93f, 0.95f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.87f, 0.90f, 0.93f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);

  colors[ImGuiCol_MenuBarBg] = ImVec4(0.91f, 0.93f, 0.95f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.45f, 0.51f, 0.61f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.43f, 0.55f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.30f, 0.37f, 0.49f, 1.00f);

  colors[ImGuiCol_Header] = ImVec4(0.86f, 0.89f, 0.93f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.81f, 0.85f, 0.90f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.81f, 0.87f, 1.00f);

  colors[ImGuiCol_Tab] = ImVec4(0.86f, 0.89f, 0.93f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.81f, 0.86f, 0.93f, 1.00f);
  colors[ImGuiCol_TabSelected] = ImVec4(0.98f, 0.98f, 0.99f, 1.00f);
  colors[ImGuiCol_TabDimmed] = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
  colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);

  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.88f, 0.91f, 0.94f, 1.00f);
  colors[ImGuiCol_TableBorderStrong] = ImVec4(0.73f, 0.77f, 0.82f, 1.00f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.84f, 0.87f, 0.91f, 1.00f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.28f, 0.36f, 0.48f, 0.04f);

  colors[ImGuiCol_DockingPreview] = ImVec4(0.20f, 0.45f, 0.90f, 0.35f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.73f, 0.77f, 0.82f, 1.00f);
  colors[ImGuiCol_InputTextCursor] = ImVec4(0.08f, 0.11f, 0.16f, 1.00f);
  colors[ImGuiCol_TextLink] = ImVec4(0.06f, 0.23f, 0.58f, 1.00f);
  colors[ImGuiCol_TreeLines] = ImVec4(0.36f, 0.43f, 0.54f, 1.00f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.24f, 0.31f, 0.25f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.24f, 0.31f, 0.35f);
}

auto Theme::loadFonts()
    -> std::pair<std::reference_wrapper<ImFont>,
                 std::reference_wrapper<ImFont>> {
  auto &atlas = *ImGui::GetIO().Fonts;
  ImFontConfig fontConfig{};
  // Embedded bytes outlive the context; ImGui must not free them.
  fontConfig.FontDataOwnedByAtlas = false;
  std::snprintf(fontConfig.Name, sizeof(fontConfig.Name), "Karla (interface)");
  if (!atlas.AddFontFromMemoryTTF(
          interfaceFontData, static_cast<int>(sizeof(interfaceFontData)),
          0.0f, &fontConfig)) {
    VKR_UI_ERROR("Failed to load the interface font");
  }
  auto &interfaceFont = *atlas.Fonts.back();
  std::snprintf(fontConfig.Name, sizeof(fontConfig.Name), "Cousine (code)");
  if (!atlas.AddFontFromMemoryTTF(
          codeFontData, static_cast<int>(sizeof(codeFontData)),
          0.0f, &fontConfig)) {
    VKR_UI_ERROR("Failed to load the code font");
  }
  return {interfaceFont, *atlas.Fonts.back()};
}

void Theme::apply(const ThemeDesc &config, float dpiScale) {
  if (!config.isValid() || !std::isfinite(dpiScale) || dpiScale <= 0.0f) {
    VKR_UI_ERROR("Invalid theme or DPI scale");
  }

  auto &style = ImGui::GetStyle();
  // Rebuild from unscaled metrics: repeated DPI/density changes cannot drift.
  style = ImGuiStyle{};
  if (config.dark) {
    ImGui::StyleColorsDark(&style);
  } else {
    ImGui::StyleColorsLight(&style);
  }

  if (config.dark) {
    applyDarkBase(style);
  } else {
    applyLightBase(style);
  }

  const ImVec4 accent = accentColor(config.accent);
  const ImVec4 accentHover = accentHoverColor(config.accent);
  const ImVec4 accentActive = accentActiveColor(config.accent);
  const float headerAlpha = config.dark ? 0.18f : 0.12f;
  const float headerHoverAlpha = config.dark ? 0.24f : 0.16f;
  const float headerActiveAlpha = config.dark ? 0.32f : 0.22f;
  const float selectionAlpha = config.dark ? 0.30f : 0.22f;

  auto &colors = style.Colors;

  colors[ImGuiCol_CheckMark] = accent;
  colors[ImGuiCol_SliderGrab] = accent;
  colors[ImGuiCol_SliderGrabActive] = accentActive;

  // Neutral controls keep the image and code dominant; accent marks state.
  colors[ImGuiCol_Button] = colors[ImGuiCol_FrameBg];
  colors[ImGuiCol_ButtonHovered] = colors[ImGuiCol_FrameBgHovered];
  colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_FrameBgActive];

  colors[ImGuiCol_SeparatorHovered] = accentHover;
  colors[ImGuiCol_SeparatorActive] = accentActive;

  colors[ImGuiCol_ResizeGrip] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
  colors[ImGuiCol_ResizeGripHovered] =
      ImVec4(accentHover.x, accentHover.y, accentHover.z, 0.60f);
  colors[ImGuiCol_ResizeGripActive] =
      ImVec4(accentActive.x, accentActive.y, accentActive.z, 0.90f);

  colors[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, headerAlpha);
  colors[ImGuiCol_HeaderHovered] =
      ImVec4(accent.x, accent.y, accent.z, headerHoverAlpha);
  colors[ImGuiCol_HeaderActive] =
      ImVec4(accent.x, accent.y, accent.z, headerActiveAlpha);

  colors[ImGuiCol_TabHovered] = colors[ImGuiCol_FrameBgHovered];
  colors[ImGuiCol_TabSelected] = colors[ImGuiCol_WindowBg];
  colors[ImGuiCol_TabDimmedSelected] = colors[ImGuiCol_WindowBg];
  colors[ImGuiCol_TabSelectedOverline] =
      ImVec4(accent.x, accent.y, accent.z, config.dark ? 0.80f : 0.65f);
  colors[ImGuiCol_TabDimmedSelectedOverline] =
      ImVec4(accent.x, accent.y, accent.z, config.dark ? 0.45f : 0.38f);

  colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.45f);
  colors[ImGuiCol_TextSelectedBg] =
      ImVec4(accent.x, accent.y, accent.z, selectionAlpha);
  colors[ImGuiCol_NavCursor] = accent;

  const float rounding = config.rounding;
  style.Alpha = config.alpha;
  style.WindowRounding = rounding;
  style.ChildRounding = rounding;
  style.FrameRounding = rounding;
  style.PopupRounding = rounding;
  style.ScrollbarRounding = rounding;
  style.GrabRounding = rounding;
  style.TabRounding = rounding;

  const bool compact = config.density == ThemeDensity::Compact;
  style.WindowPadding = compact ? ImVec2(10.0f, 8.0f) : ImVec2(12.0f, 10.0f);
  style.FramePadding = compact ? ImVec2(6.0f, 3.0f) : ImVec2(8.0f, 6.0f);
  style.CellPadding = compact ? ImVec2(6.0f, 3.0f) : ImVec2(8.0f, 5.0f);
  style.ItemSpacing = compact ? ImVec2(6.0f, 5.0f) : ImVec2(8.0f, 8.0f);
  style.ItemInnerSpacing = compact ? ImVec2(5.0f, 4.0f) : ImVec2(6.0f, 5.0f);
  style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
  style.IndentSpacing = compact ? 16.0f : 18.0f;
  style.ScrollbarSize = compact ? 11.0f : 13.0f;
  style.GrabMinSize = compact ? 10.0f : 12.0f;
  style.TabBarBorderSize = 0.0f;
  style.TabBarOverlineSize = 2.0f;
  style.DockingSeparatorSize = compact ? 2.0f : 3.0f;
  style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
  style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
  style.SelectableTextAlign = ImVec2(0.0f, 0.5f);
  style.SeparatorTextBorderSize = 1.0f;
  style.SeparatorTextPadding = ImVec2(0.0f, 4.0f);
  style.WindowMinSize = ImVec2(140.0f, 100.0f);

  style.WindowBorderSize = 0.0f;
  style.ChildBorderSize = 0.0f;
  style.FrameBorderSize = 0.0f;
  style.PopupBorderSize = 1.0f;
  style.TabBorderSize = 0.0f;

  style.FontSizeBase = 15.0f;
  style.FontScaleMain = config.scale;
  style.FontScaleDpi = dpiScale;
  style.ScaleAllSizes(config.scale * dpiScale);
}

} // namespace vkr::ui
