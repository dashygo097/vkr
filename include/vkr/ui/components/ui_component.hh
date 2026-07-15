#pragma once

#include <imgui.h>
#include <string>
#include <utility>

namespace vkr::ui {

class UiComponent {
public:
  explicit UiComponent(std::string name, bool defaultOpen = true);
  virtual ~UiComponent() = default;

  UiComponent(const UiComponent &) = delete;
  auto operator=(const UiComponent &) -> UiComponent & = delete;

  [[nodiscard]] auto name() const noexcept -> const std::string & {
    return name_;
  }

  [[nodiscard]] auto open() const noexcept -> bool { return open_; }
  [[nodiscard]] auto openRef() noexcept -> bool & { return open_; }

  void resetOpen() noexcept { open_ = default_open_; }

  virtual void renderWindow();

protected:
  virtual void render() = 0;

  [[nodiscard]] virtual auto windowFlags() const noexcept -> ImGuiWindowFlags {
    return ImGuiWindowFlags_None;
  }

private:
  std::string name_;
  bool open_{true};
  bool default_open_{true};
};

} // namespace vkr::ui
