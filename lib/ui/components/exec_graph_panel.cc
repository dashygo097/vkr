#include "vkr/ui/components/exec_graph_panel.hh"
#include "vkr/exec/capability.hh"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace vkr::ui {
namespace {

constexpr ImVec2 nodeSize{16.0f, 5.4f};
constexpr ImVec2 nodeSpacing{4.5f, 2.3f};

} // namespace

ExecGraphPanel::ExecGraphPanel(const exec::Graph &graph,
                               const Selection &selection,
                               std::function<void(Selection)> onSelect)
    : UiComponent("Exec Graph"), selection_(selection),
      on_select_(std::move(onSelect)) {
  const auto order = graph.executionOrder();
  passes_.reserve(order.size());
  std::unordered_map<std::string, size_t> indices{};
  indices.reserve(order.size());

  for (const auto &pass : order) {
    std::vector<size_t> dependencies{};
    const auto producers = graph.dependencies(pass.get());
    dependencies.reserve(producers.size());
    for (const auto &producer : producers) {
      dependencies.push_back(indices.at(producer.get().name()));
    }
    indices.emplace(pass.get().name(), passes_.size());
    passes_.push_back({
        pass,
        pass.get().capability<exec::GraphicsPipelineCapability>().has_value(),
        pass.get().capability<exec::RenderTargetCapability>().has_value(),
        pass.get().capability<exec::PresentCapability>().has_value(),
        std::move(dependencies),
        0,
        {},
    });
  }

  layoutGraph();
}

void ExecGraphPanel::layoutGraph() {
  if (passes_.empty()) {
    return;
  }

  std::vector<size_t> levels(passes_.size(), 0);
  std::vector<std::vector<size_t>> layers(passes_.size());
  std::vector<std::vector<size_t>> consumers(passes_.size());
  size_t layerCount = 0;
  size_t widestLayer = 0;
  size_t edgeCount = 0;
  for (size_t index = 0; index < passes_.size(); ++index) {
    for (const size_t producer : passes_[index].dependencies) {
      levels[index] = std::max(levels[index], levels[producer] + 1);
      consumers[producer].push_back(index);
      ++passes_[producer].consumerCount;
      ++edgeCount;
    }
    auto &layer = layers[levels[index]];
    layer.push_back(index);
    widestLayer = std::max(widestLayer, layer.size());
    layerCount = std::max(layerCount, levels[index] + 1);
  }
  layers.resize(layerCount);

  auto alignLayer = [&](size_t level) {
    const auto &layer = layers[level];
    const float offset = static_cast<float>(widestLayer - layer.size()) * 0.5f;
    for (size_t row = 0; row < layer.size(); ++row) {
      passes_[layer[row]].position =
          ImVec2(static_cast<float>(level), offset + static_cast<float>(row));
    }
  };
  for (size_t level = 0; level < layerCount; ++level) {
    alignLayer(level);
  }

  auto reorderLayer = [&](size_t level, bool forward) {
    auto score = [&](size_t index) {
      const auto &neighbors =
          forward ? passes_[index].dependencies : consumers[index];
      if (neighbors.empty()) {
        return passes_[index].position.y;
      }
      float sum = 0.0f;
      for (const size_t neighbor : neighbors) {
        sum += passes_[neighbor].position.y;
      }
      return sum / static_cast<float>(neighbors.size());
    };
    auto &layer = layers[level];
    std::stable_sort(layer.begin(), layer.end(),
                     [&](size_t a, size_t b) { return score(a) < score(b); });
    alignLayer(level);
  };
  for (size_t sweep = 0; sweep < 4; ++sweep) {
    for (size_t level = 1; level < layerCount; ++level) {
      reorderLayer(level, true);
    }
    for (size_t level = layerCount - 1; level > 0; --level) {
      reorderLayer(level - 1, false);
    }
  }

  const ImVec2 stride{nodeSize.x + nodeSpacing.x, nodeSize.y + nodeSpacing.y};
  for (auto &entry : passes_) {
    entry.position.x *= stride.x;
    entry.position.y *= stride.y;
  }
  graph_max_ = {static_cast<float>(layerCount) * stride.x - nodeSpacing.x,
                static_cast<float>(widestLayer) * stride.y - nodeSpacing.y};

  edges_.reserve(edgeCount);
  size_t outerLane = 0;
  for (size_t consumer = 0; consumer < passes_.size(); ++consumer) {
    const auto &target = passes_[consumer];
    for (const size_t producer : target.dependencies) {
      const auto &source = passes_[producer];
      const ImVec2 start{source.position.x + nodeSize.x,
                         source.position.y + nodeSize.y * 0.5f};
      const ImVec2 end{target.position.x,
                       target.position.y + nodeSize.y * 0.5f};
      EdgeEntry edge{};
      edge.producer = producer;
      edge.consumer = consumer;
      if (levels[consumer] == levels[producer] + 1) {
        if (start.y == end.y) {
          edge.points[0] = start;
          edge.points[1] = end;
          edge.pointCount = 2;
        } else {
          const float middle = (start.x + end.x) * 0.5f;
          edge.points = {{start, {middle, start.y}, {middle, end.y}, end}};
          edge.pointCount = 4;
        }
      } else {
        const float lane =
            -nodeSpacing.y * 0.5f - static_cast<float>(outerLane++) * 0.7f;
        const float exitX = start.x + nodeSpacing.x * 0.35f;
        const float enterX = end.x - nodeSpacing.x * 0.35f;
        edge.points = {{start,
                        {exitX, start.y},
                        {exitX, lane},
                        {enterX, lane},
                        {enterX, end.y},
                        end}};
        edge.pointCount = 6;
        graph_min_.y = std::min(graph_min_.y, lane);
      }
      edges_.push_back(edge);
    }
  }
}

void ExecGraphPanel::render() {
  const float contentRight =
      ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
  ImGui::BeginDisabled(passes_.empty());
  if (ImGui::Button("Fit View")) {
    fit_view_ = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("1:1")) {
    zoom_ = 1.0f;
    view_initialized_ = true;
    fit_view_ = false;
    const float fontSize = ImGui::GetFontSize();
    view_offset_ = {(2.0f - graph_min_.x) * fontSize,
                    (2.0f - graph_min_.y) * fontSize};
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::TextDisabled("%.0f%%", zoom_ * 100.0f);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(
        "Left to right: dependency direction\n"
        "Wheel: zoom at cursor\nDrag empty space: pan\n"
        "Middle/right drag: pan anywhere\nClick a pass: inspect\n"
        "Layers indicate dependencies, not parallel GPU execution.");
  }

  std::array<char, 64> summary{};
  std::snprintf(summary.data(), summary.size(), "%zu passes / %zu links",
                passes_.size(), edges_.size());
  const float summaryWidth = ImGui::CalcTextSize(summary.data()).x;
  const float summaryX = contentRight - summaryWidth;
  if (summaryX > ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x) {
    ImGui::SameLine(summaryX - ImGui::GetWindowPos().x);
    ImGui::TextDisabled("%s", summary.data());
  }
  ImGui::Separator();

  const float available = ImGui::GetContentRegionAvail().y;
  const bool showHint = available > ImGui::GetFontSize() * 9.0f;
  const float hintHeight = showHint ? ImGui::GetTextLineHeightWithSpacing() : 0;
  renderGraph(std::max(1.0f, available - hintHeight));
  if (showHint) {
    ImGui::TextDisabled("Dependencies ->");
    const std::string_view hint{
        "Scroll to zoom / Drag to pan / Click to inspect"};
    if (contentRight > ImGui::GetItemRectMax().x +
                           ImGui::GetStyle().ItemSpacing.x +
                           ImGui::CalcTextSize(hint.data()).x) {
      ImGui::SameLine();
      ImGui::TextDisabled("%s", hint.data());
    }
  }
}

void ExecGraphPanel::renderGraph(float height) {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  const bool visible = ImGui::BeginChild(
      "##pass_graph", ImVec2(0.0f, height), ImGuiChildFlags_Borders,
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
  ImGui::PopStyleVar();
  if (!visible) {
    ImGui::EndChild();
    return;
  }

  const float fontSize = ImGui::GetFontSize();
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  const ImVec2 available = ImGui::GetContentRegionAvail();
  const ImVec2 canvasSize{std::max(1.0f, available.x),
                          std::max(1.0f, available.y)};
  const ImVec2 canvasEnd{origin.x + canvasSize.x, origin.y + canvasSize.y};
  ImGui::InvisibleButton("##canvas", canvasSize,
                         ImGuiButtonFlags_MouseButtonLeft |
                             ImGuiButtonFlags_MouseButtonMiddle |
                             ImGuiButtonFlags_MouseButtonRight);
  const bool hovered = ImGui::IsItemHovered();
  const bool active = ImGui::IsItemActive();
  const auto &io = ImGui::GetIO();

  if ((!view_initialized_ || fit_view_) && !passes_.empty()) {
    const float padding = fontSize * 2.0f;
    const float width = (graph_max_.x - graph_min_.x) * fontSize;
    const float graphHeight = (graph_max_.y - graph_min_.y) * fontSize;
    const float fittedZoom =
        std::clamp(std::min((canvasSize.x - padding * 2.0f) / width,
                            (canvasSize.y - padding * 2.0f) / graphHeight),
                   0.02f, 1.0f);
    zoom_ = !view_initialized_ && !fit_view_ ? std::max(0.65f, fittedZoom)
                                             : fittedZoom;
    const float scale = fontSize * zoom_;
    view_offset_ = {
        (canvasSize.x - (graph_max_.x - graph_min_.x) * scale) * 0.5f -
            graph_min_.x * scale,
        (canvasSize.y - (graph_max_.y - graph_min_.y) * scale) * 0.5f -
            graph_min_.y * scale};
    if (zoom_ > fittedZoom) {
      const auto &first = passes_.front().position;
      view_offset_ = {padding - first.x * scale,
                      (canvasSize.y - nodeSize.y * scale) * 0.5f -
                          first.y * scale};
    }
    view_initialized_ = true;
    fit_view_ = false;
  }

  if (hovered) {
    ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
    if (io.MouseWheel != 0.0f) {
      const float nextZoom =
          std::clamp(zoom_ * std::pow(1.15f, io.MouseWheel), 0.02f, 2.5f);
      const float ratio = nextZoom / zoom_;
      const ImVec2 mouse{io.MousePos.x - origin.x, io.MousePos.y - origin.y};
      view_offset_.x = mouse.x - (mouse.x - view_offset_.x) * ratio;
      view_offset_.y = mouse.y - (mouse.y - view_offset_.y) * ratio;
      zoom_ = nextZoom;
      fit_view_ = false;
    }
  }

  const float scale = fontSize * zoom_;
  std::optional<size_t> hoveredNode{};
  if (hovered) {
    const ImVec2 mouse{(io.MousePos.x - origin.x - view_offset_.x) / scale,
                       (io.MousePos.y - origin.y - view_offset_.y) / scale};
    for (size_t index = 0; index < passes_.size(); ++index) {
      const auto &position = passes_[index].position;
      if (mouse.x >= position.x - 0.3f &&
          mouse.x <= position.x + nodeSize.x + 0.3f && mouse.y >= position.y &&
          mouse.y <= position.y + nodeSize.y) {
        hoveredNode = index;
        break;
      }
    }
  }
  if (ImGui::IsItemActivated()) {
    dragging_canvas_ = !hoveredNode ||
                       ImGui::IsMouseDown(ImGuiMouseButton_Middle) ||
                       ImGui::IsMouseDown(ImGuiMouseButton_Right);
  }
  if (hoveredNode && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    on_select_({SelectionType::Pass, passes_[*hoveredNode].pass.get().name()});
  }
  if (active && dragging_canvas_ &&
      (ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
       ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
       ImGui::IsMouseDragging(ImGuiMouseButton_Right))) {
    view_offset_.x += io.MouseDelta.x;
    view_offset_.y += io.MouseDelta.y;
    fit_view_ = false;
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
  }
  if (!active) {
    dragging_canvas_ = false;
  }

  drawGraph(origin, canvasEnd, scale, hoveredNode);

  if (hoveredNode && !dragging_canvas_ &&
      ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) &&
      !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    const auto &entry = passes_[*hoveredNode];
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(entry.pass.get().name().c_str());
    ImGui::TextDisabled("%zu predecessors / %zu consumers",
                        entry.dependencies.size(), entry.consumerCount);
    ImGui::Separator();
    if (entry.pipeline) {
      ImGui::BulletText("Graphics pipeline");
    }
    if (entry.target) {
      ImGui::BulletText("Render target");
    }
    if (entry.presents) {
      ImGui::BulletText("Presentation endpoint");
    }
    ImGui::EndTooltip();
  }
  ImGui::EndChild();
}

void ExecGraphPanel::drawGraph(ImVec2 origin, ImVec2 canvasEnd, float scale,
                               std::optional<size_t> hoveredNode) {
  auto screenPosition = [&](ImVec2 point) {
    return ImVec2(origin.x + view_offset_.x + point.x * scale,
                  origin.y + view_offset_.y + point.y * scale);
  };
  auto &drawList = *ImGui::GetWindowDrawList();
  const auto &style = ImGui::GetStyle();
  const ImU32 accent = ImGui::GetColorU32(ImGuiCol_CheckMark);
  ImGui::PushClipRect(origin, canvasEnd, true);

  float gridStep = scale * 2.0f;
  while (gridStep < 24.0f) {
    gridStep *= 2.0f;
  }
  while (gridStep > 48.0f) {
    gridStep *= 0.5f;
  }
  const ImU32 gridColor = ImGui::GetColorU32(ImGuiCol_Border, 0.22f);
  const float gridX = origin.x + std::fmod(view_offset_.x, gridStep);
  const float gridY = origin.y + std::fmod(view_offset_.y, gridStep);
  for (float x = gridX; x <= canvasEnd.x; x += gridStep) {
    drawList.AddLine({x, origin.y}, {x, canvasEnd.y}, gridColor);
  }
  for (float y = gridY; y <= canvasEnd.y; y += gridStep) {
    drawList.AddLine({origin.x, y}, {canvasEnd.x, y}, gridColor);
  }

  if (passes_.empty()) {
    const std::string_view message{"No compiled passes"};
    const ImVec2 size = ImGui::CalcTextSize(message.data());
    drawList.AddText({origin.x + (canvasEnd.x - origin.x - size.x) * 0.5f,
                      origin.y + (canvasEnd.y - origin.y - size.y) * 0.5f},
                     ImGui::GetColorU32(ImGuiCol_TextDisabled), message.data());
    ImGui::PopClipRect();
    return;
  }

  const float portRadius = std::clamp(scale * 0.19f, 1.0f, 4.0f);

  std::optional<size_t> selectedPass{};
  if (selection_.type == SelectionType::Pass) {
    for (size_t index = 0; index < passes_.size(); ++index) {
      if (passes_[index].pass.get().name() == selection_.name) {
        selectedPass = index;
        break;
      }
    }
  }

  for (int highlight = 0; highlight < 2; ++highlight) {
    for (const auto &edge : edges_) {
      const bool selected = selectedPass && (edge.producer == *selectedPass ||
                                             edge.consumer == *selectedPass);
      if (selected != (highlight != 0)) {
        continue;
      }
      std::array<ImVec2, 6> points{};
      ImVec2 minimum = screenPosition(edge.points[0]);
      ImVec2 maximum = minimum;
      for (size_t index = 0; index < edge.pointCount; ++index) {
        points[index] = screenPosition(edge.points[index]);
        minimum.x = std::min(minimum.x, points[index].x);
        minimum.y = std::min(minimum.y, points[index].y);
        maximum.x = std::max(maximum.x, points[index].x);
        maximum.y = std::max(maximum.y, points[index].y);
      }
      const float margin = std::max(1.0f, scale * 0.35f);
      if (maximum.x + margin < origin.x || minimum.x - margin > canvasEnd.x ||
          maximum.y + margin < origin.y || minimum.y - margin > canvasEnd.y) {
        continue;
      }
      const ImVec2 tip = points[edge.pointCount - 1];
      const bool showArrow = scale >= 4.0f;
      const float arrow =
          showArrow ? std::clamp(scale * 0.28f, 2.0f, 5.0f) : 0.0f;
      const ImVec2 arrowTip{tip.x - (showArrow ? portRadius + 1.0f : 0.0f),
                            tip.y};
      points[edge.pointCount - 1].x = arrowTip.x - arrow;
      const ImU32 color =
          selected ? accent
                   : ImGui::GetColorU32(ImGuiCol_TextDisabled,
                                        selectedPass ? 0.32f : 0.65f);
      drawList.PathLineTo(points[0]);
      for (size_t index = 1; index + 1 < edge.pointCount; ++index) {
        const ImVec2 corner = points[index];
        const ImVec2 incoming{points[index - 1].x - corner.x,
                              points[index - 1].y - corner.y};
        const ImVec2 outgoing{points[index + 1].x - corner.x,
                              points[index + 1].y - corner.y};
        const float inLength = std::hypot(incoming.x, incoming.y);
        const float outLength = std::hypot(outgoing.x, outgoing.y);
        const float radius =
            std::min(scale * 0.3f, std::min(inLength, outLength) * 0.25f);
        if (radius > 0.0f) {
          drawList.PathLineTo({corner.x + incoming.x / inLength * radius,
                               corner.y + incoming.y / inLength * radius});
          drawList.PathBezierQuadraticCurveTo(
              corner, {corner.x + outgoing.x / outLength * radius,
                       corner.y + outgoing.y / outLength * radius});
        } else {
          drawList.PathLineTo(corner);
        }
      }
      drawList.PathLineTo(points[edge.pointCount - 1]);
      drawList.PathStroke(color, ImDrawFlags_None, selected ? 2.0f : 1.2f);
      if (showArrow) {
        drawList.AddTriangleFilled(
            arrowTip, {arrowTip.x - arrow, arrowTip.y - arrow * 0.55f},
            {arrowTip.x - arrow, arrowTip.y + arrow * 0.55f}, color);
      }
    }
  }

  for (size_t index = 0; index < passes_.size(); ++index) {
    const auto &entry = passes_[index];
    const bool selected = selectedPass && index == *selectedPass;
    const bool nodeHovered = hoveredNode && *hoveredNode == index;
    const ImVec2 start = screenPosition(entry.position);
    const ImVec2 end{start.x + nodeSize.x * scale,
                     start.y + nodeSize.y * scale};
    if (end.x < origin.x || start.x > canvasEnd.x || end.y < origin.y ||
        start.y > canvasEnd.y) {
      continue;
    }
    const bool related =
        !selectedPass || selected ||
        std::find(passes_[*selectedPass].dependencies.begin(),
                  passes_[*selectedPass].dependencies.end(),
                  index) != passes_[*selectedPass].dependencies.end() ||
        std::find(entry.dependencies.begin(), entry.dependencies.end(),
                  *selectedPass) != entry.dependencies.end();
    const float rounding = std::min(style.FrameRounding * zoom_, scale);
    const float headerBottom = start.y + scale * 2.9f;
    const ImU32 border =
        selected ? accent
                 : ImGui::GetColorU32(nodeHovered ? ImGuiCol_TextDisabled
                                                  : ImGuiCol_Border);
    if (selected) {
      drawList.AddRect({start.x - 2.0f, start.y - 2.0f},
                       {end.x + 2.0f, end.y + 2.0f},
                       ImGui::GetColorU32(ImGuiCol_CheckMark, 0.18f),
                       rounding + 2.0f, ImDrawFlags_None, 3.0f);
    }
    drawList.AddRectFilled(start, end, ImGui::GetColorU32(ImGuiCol_WindowBg),
                           rounding);
    drawList.AddRectFilled(start, {end.x, headerBottom},
                           ImGui::GetColorU32(nodeHovered
                                                  ? ImGuiCol_FrameBgHovered
                                                  : ImGuiCol_FrameBg),
                           rounding, ImDrawFlags_RoundCornersTop);
    drawList.AddLine({start.x, headerBottom}, {end.x, headerBottom},
                     ImGui::GetColorU32(ImGuiCol_Border, 0.65f));
    drawList.AddRect(start, end, border, rounding, ImDrawFlags_None,
                     selected ? 1.8f : 1.0f);

    if (scale < 4.0f) {
      continue;
    }

    const float centerY = (start.y + end.y) * 0.5f;
    const ImU32 portColor =
        selected ? accent : ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.8f);
    const auto port = [&](ImVec2 position) {
      drawList.AddCircleFilled(position, portRadius + 1.0f,
                               ImGui::GetColorU32(ImGuiCol_ChildBg));
      drawList.AddCircleFilled(position, portRadius, portColor);
    };
    if (!entry.dependencies.empty()) {
      port({start.x, centerY});
    }
    if (entry.consumerCount != 0) {
      port({end.x, centerY});
    }

    auto &font = *ImGui::GetFont();
    const float titleSize = std::max(10.0f, scale);
    const float padding = scale * 0.8f;
    const ImVec2 titlePosition{start.x + padding,
                               start.y + (scale * 2.9f - titleSize) * 0.5f};
    const float titleRight = end.x - padding;
    const float titleWidth = titleRight - titlePosition.x;
    const auto &name = entry.pass.get().name();
    const bool ellipsized =
        font.CalcTextSizeA(titleSize, std::numeric_limits<float>::max(), 0.0f,
                           name.c_str())
            .x > titleWidth;
    const float ellipsisWidth =
        ellipsized
            ? font.CalcTextSizeA(titleSize, std::numeric_limits<float>::max(),
                                 0.0f, "...")
                  .x
            : 0.0f;
    const ImU32 textColor = ImGui::GetColorU32(
        related || nodeHovered ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    drawList.PushClipRect(titlePosition,
                          {titleRight - ellipsisWidth, headerBottom}, true);
    drawList.AddText(&font, titleSize, titlePosition, textColor, name.c_str());
    drawList.PopClipRect();
    if (ellipsized) {
      drawList.AddText(&font, titleSize,
                       {titleRight - ellipsisWidth, titlePosition.y}, textColor,
                       "...");
    }

    if (scale < 9.0f) {
      continue;
    }

    const float badgeSize = std::max(8.5f, scale * 0.75f);
    const float badgePadding = scale * 0.35f;
    const float badgeHeight = badgeSize + badgePadding * 2.0f;
    const float badgeY =
        headerBottom + (end.y - headerBottom - badgeHeight) * 0.5f;
    float badgeX = start.x + padding;
    const std::array<std::pair<std::string_view, bool>, 3> capabilities{{
        {"Pipeline", entry.pipeline},
        {"Target", entry.target},
        {"Present", entry.presents},
    }};
    drawList.PushClipRect({start.x + padding, headerBottom},
                          {titleRight, end.y}, true);
    for (const auto &[label, enabled] : capabilities) {
      if (!enabled) {
        continue;
      }
      const float width =
          font.CalcTextSizeA(badgeSize, std::numeric_limits<float>::max(), 0.0f,
                             label.data(), label.data() + label.size())
              .x +
          badgePadding * 2.0f;
      drawList.AddRectFilled({badgeX, badgeY},
                             {badgeX + width, badgeY + badgeHeight},
                             ImGui::GetColorU32(ImGuiCol_FrameBg), rounding);
      drawList.AddText(&font, badgeSize,
                       {badgeX + badgePadding, badgeY + badgePadding},
                       ImGui::GetColorU32(ImGuiCol_TextDisabled), label.data(),
                       label.data() + label.size());
      badgeX += width + scale * 0.3f;
    }
    if (!entry.pipeline && !entry.target && !entry.presents) {
      drawList.AddText(&font, badgeSize, {badgeX, badgeY + badgePadding},
                       ImGui::GetColorU32(ImGuiCol_TextDisabled),
                       "Custom pass");
    }
    drawList.PopClipRect();
  }
  ImGui::PopClipRect();
}

} // namespace vkr::ui
