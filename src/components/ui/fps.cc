#include "vkr/components/ui/fps.hpp"

namespace vkr {

void FPSPanel::render() {
  ImGui::Begin("Performance");
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::End();
}
} // namespace vkr
