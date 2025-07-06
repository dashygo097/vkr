#include "vkr/ui/fps.hpp"
#include <imgui.h>

void FPSPanel::draw(float fps) {
  ImGui::Begin("Performance");
  ImGui::Text("FPS: %.1f", fps);
  ImGui::End();
}
