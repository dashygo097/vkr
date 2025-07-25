#include "vkr/components/ui/fps.hpp"

namespace vkr {

void FPSPanel::render(float fps) {
  ImGui::Begin("Performance");
  ImGui::Text("FPS: %.1f", fps);
  ImGui::End();
}
} // namespace vkr
