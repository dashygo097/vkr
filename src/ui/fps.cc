#include "vkr/ui/fps.hpp"
#include <imgui.h>

namespace vkr {

void FPSPanel::render(float fps) {
  ImGui::Begin("Performance");
  ImGui::Text("FPS: %.1f", fps);
  ImGui::End();
}
} // namespace vkr
