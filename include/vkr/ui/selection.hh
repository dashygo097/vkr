#pragma once

#include <string>

namespace vkr::ui {

enum class SelectionType {
  None,
  Mesh,
  UniformBuffer,
  Texture,
  Cubemap,
  Pass,
};

struct Selection {
  SelectionType type{SelectionType::None};
  std::string name{};
};

} // namespace vkr::ui
