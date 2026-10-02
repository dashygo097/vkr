#include "vkr/ui/components/exec_graph_panel.hh"
#include "vkr/exec/render/targets/offscreen.hh"
#include "vkr/pipeline/graphics_pipeline.hh"
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <string>
#include <unordered_map>
#include <utility>

namespace vkr::ui {
namespace {

constexpr ImVec2 nodeSize{11.0f, 2.4f};
constexpr ImVec2 nodeSpacing{3.5f, 3.0f};

} // namespace

ExecGraphPanel::ExecGraphPanel(const exec::Graph &graph)
    : UiComponent("Exec Graph") {
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
        pass.get().capability<exec::GraphicsPipelineCapability>(),
        pass.get().capability<exec::RenderTargetCapability>(),
        pass.get().capability<exec::PresentCapability>().has_value(),
        std::move(dependencies),
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
          ImVec2(offset + static_cast<float>(row), static_cast<float>(level));
    }
  };
  for (size_t level = 0; level < layerCount; ++level) {
    alignLayer(level);
  }

  // Barycentric sweeps reduce crossings without changing graph or pass order.
  auto reorderLayer = [&](size_t level, bool forward) {
    auto score = [&](size_t index) {
      const auto &neighbors =
          forward ? passes_[index].dependencies : consumers[index];
      if (neighbors.empty()) {
        return passes_[index].position.x;
      }
      float sum = 0.0f;
      for (const size_t neighbor : neighbors) {
        sum += passes_[neighbor].position.x;
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

  const ImVec2 stride{nodeSize.x + nodeSpacing.x,
                     nodeSize.y + nodeSpacing.y};
  for (auto &entry : passes_) {
    entry.position.x *= stride.x;
    entry.position.y *= stride.y;
  }
  graph_max_ = ImVec2(static_cast<float>(widestLayer) * stride.x - nodeSpacing.x,
                     static_cast<float>(layerCount) * stride.y - nodeSpacing.y);

  edges_.reserve(edgeCount);
  size_t outerLane = 0;
  for (size_t consumer = 0; consumer < passes_.size(); ++consumer) {
    const auto &target = passes_[consumer];
    for (const size_t producer : target.dependencies) {
      const auto &source = passes_[producer];
      const ImVec2 start{source.position.x + nodeSize.x * 0.5f,
                        source.position.y + nodeSize.y};
      const ImVec2 end{target.position.x + nodeSize.x * 0.5f,
                       target.position.y};
      EdgeEntry edge{};
      edge.producer = producer;
      edge.consumer = consumer;
      if (levels[consumer] == levels[producer] + 1) {
        if (start.x == end.x) {
          edge.points[0] = start;
          edge.points[1] = end;
          edge.pointCount = 2;
        } else {
          const float middle = (start.y + end.y) * 0.5f;
          edge.points = {{start, {start.x, middle}, {end.x, middle}, end}};
          edge.pointCount = 4;
        }
      } else {
        // Skip-layer edges use outer lanes, never cut through intervening nodes.
        const float lane =
            -nodeSpacing.x * 0.5f - static_cast<float>(outerLane++) * 0.7f;
        const float exitY = start.y + nodeSpacing.y * 0.35f;
        const float enterY = end.y - nodeSpacing.y * 0.35f;
        edge.points = {{start, {start.x, exitY}, {lane, exitY}, {lane, enterY},
                        {end.x, enterY}, end}};
        edge.pointCount = 6;
        graph_min_.x = std::min(graph_min_.x, lane);
      }
      edges_.push_back(edge);
    }
  }
}

void ExecGraphPanel::render() {
  if (ImGui::Button("Fit View")) {
    fit_view_ = true;
  }
  ImGui::SameLine();
  ImGui::Checkbox("Details", &show_details_);
  ImGui::SameLine();
  ImGui::TextDisabled("%.0f%%", zoom_ * 100.0f);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(
        "%zu passes, %zu dependency edges\n"
        "Wheel: zoom at cursor\nDrag empty space: pan\n"
        "Middle/right drag: pan anywhere\nClick a pass: inspect\n"
        "Layers indicate dependencies, not parallel GPU execution.",
        passes_.size(), edges_.size());
  }

  if (passes_.empty()) {
    ImGui::TextDisabled("No passes");
    return;
  }

  const float available = ImGui::GetContentRegionAvail().y;
  const float height =
      show_details_ ? std::max(ImGui::GetFontSize() * 6.0f, available * 0.6f)
                    : std::max(1.0f, available);
  renderGraph(height);
  if (show_details_) {
    const auto &entry = passes_[selected_pass_];
    ImGui::SeparatorText(entry.pass.get().name().c_str());
    ImGui::PushID(entry.pass.get().name().c_str());
    renderPass(entry);
    ImGui::PopID();
  }
}

void ExecGraphPanel::renderGraph(float height) {
  if (!ImGui::BeginChild("##pass_graph", ImVec2(0.0f, height),
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
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

  if (fit_view_) {
    const float padding = fontSize;
    const float width = (graph_max_.x - graph_min_.x) * fontSize;
    const float graphHeight = (graph_max_.y - graph_min_.y) * fontSize;
    zoom_ = std::clamp(
        std::min((canvasSize.x - padding * 2.0f) / width,
                 (canvasSize.y - padding * 2.0f) / graphHeight),
        0.02f, 1.0f);
    const float scale = fontSize * zoom_;
    view_offset_ = {
        (canvasSize.x - (graph_max_.x - graph_min_.x) * scale) * 0.5f -
            graph_min_.x * scale,
        (canvasSize.y - (graph_max_.y - graph_min_.y) * scale) * 0.5f -
            graph_min_.y * scale};
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
    const ImVec2 mouse{
        (io.MousePos.x - origin.x - view_offset_.x) / scale,
        (io.MousePos.y - origin.y - view_offset_.y) / scale};
    for (size_t index = 0; index < passes_.size(); ++index) {
      const auto &position = passes_[index].position;
      if (mouse.x >= position.x && mouse.x <= position.x + nodeSize.x &&
          mouse.y >= position.y && mouse.y <= position.y + nodeSize.y) {
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
    selected_pass_ = *hoveredNode;
    show_details_ = true;
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

  if (hoveredNode && !dragging_canvas_) {
    const auto &entry = passes_[*hoveredNode];
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(entry.pass.get().name().c_str());
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

  // Draw selected connections last so unrelated edges cannot obscure them.
  for (int highlight = 0; highlight < 2; ++highlight) {
    for (const auto &edge : edges_) {
      const bool selected =
          edge.producer == selected_pass_ || edge.consumer == selected_pass_;
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
      const float arrow = scale * 0.35f;
      points[edge.pointCount - 1].y -= arrow;
      const ImU32 color =
          selected ? accent : ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.6f);
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
          drawList.PathLineTo(
              {corner.x + incoming.x / inLength * radius,
               corner.y + incoming.y / inLength * radius});
          drawList.PathBezierQuadraticCurveTo(
              corner, {corner.x + outgoing.x / outLength * radius,
                       corner.y + outgoing.y / outLength * radius});
        } else {
          drawList.PathLineTo(corner);
        }
      }
      drawList.PathLineTo(points[edge.pointCount - 1]);
      drawList.PathStroke(color, ImDrawFlags_None, selected ? 1.8f : 1.0f);
      drawList.AddTriangleFilled(
          tip, {tip.x - arrow * 0.55f, tip.y - arrow},
          {tip.x + arrow * 0.55f, tip.y - arrow}, color);
    }
  }

  const auto &selectedDependencies = passes_[selected_pass_].dependencies;
  for (size_t index = 0; index < passes_.size(); ++index) {
    const auto &entry = passes_[index];
    const bool selected = index == selected_pass_;
    const bool nodeHovered = hoveredNode && *hoveredNode == index;
    const ImVec2 start = screenPosition(entry.position);
    const ImVec2 end{start.x + nodeSize.x * scale,
                    start.y + nodeSize.y * scale};
    if (end.x < origin.x || start.x > canvasEnd.x ||
        end.y < origin.y || start.y > canvasEnd.y) {
      continue;
    }
    const ImU32 fill = ImGui::GetColorU32(
        selected ? ImGuiCol_HeaderActive
                 : (nodeHovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg));
    drawList.AddRectFilled(start, end, fill, style.FrameRounding);
    drawList.AddRect(start, end,
                    selected ? accent : ImGui::GetColorU32(ImGuiCol_Border),
                    style.FrameRounding, ImDrawFlags_None,
                    selected ? 1.5f : 1.0f);

    const int capabilityCount = static_cast<int>(entry.pipeline.has_value()) +
                                static_cast<int>(entry.target.has_value()) +
                                static_cast<int>(entry.presents);
    float dotX = end.x - scale * 0.55f;
    const float centerY = (start.y + end.y) * 0.5f;
    auto dot = [&](ImGuiCol color) {
      drawList.AddCircleFilled({dotX, centerY}, scale * 0.14f,
                               ImGui::GetColorU32(color));
      dotX -= scale * 0.45f;
    };
    if (entry.presents) {
      dot(ImGuiCol_CheckMark);
    }
    if (entry.target) {
      dot(ImGuiCol_PlotHistogram);
    }
    if (entry.pipeline) {
      dot(ImGuiCol_PlotLines);
    }

    if (scale >= 4.0f) {
      const bool related =
          selected ||
          std::find(selectedDependencies.begin(), selectedDependencies.end(),
                    index) != selectedDependencies.end() ||
          std::find(entry.dependencies.begin(), entry.dependencies.end(),
                    selected_pass_) != entry.dependencies.end();
      const float labelSize = std::max(8.0f, scale);
      const ImVec2 textStart{start.x + scale * 0.5f,
                            centerY - labelSize * 0.5f};
      const ImVec2 textEnd{
          end.x - scale * (0.5f + static_cast<float>(capabilityCount) * 0.45f),
          end.y};
      drawList.PushClipRect(start, textEnd, true);
      drawList.AddText(ImGui::GetFont(), labelSize, textStart,
                       ImGui::GetColorU32(related || nodeHovered
                                            ? ImGuiCol_Text
                                            : ImGuiCol_TextDisabled),
                       entry.pass.get().name().c_str());
      drawList.PopClipRect();
    }
  }
  ImGui::PopClipRect();
}

void ExecGraphPanel::renderPass(const PassEntry &entry) {
  const auto &pass = entry.pass.get();
  ImGui::TextUnformatted("Capabilities:");
  if (entry.pipeline) {
    ImGui::BulletText("Graphics pipeline");
  }
  if (entry.target) {
    ImGui::BulletText("Render target");
  }
  if (entry.presents) {
    ImGui::BulletText("Presentation endpoint (after submit)");
  }
  if (!entry.pipeline && !entry.target && !entry.presents) {
    ImGui::TextDisabled("No inspected capabilities");
  }

  if (ImGui::TreeNodeEx("Dependencies", ImGuiTreeNodeFlags_SpanAvailWidth)) {
    if (entry.dependencies.empty()) {
      ImGui::TextDisabled("None");
    } else {
      for (const size_t producer : entry.dependencies) {
        ImGui::BulletText("%s", passes_[producer].pass.get().name().c_str());
      }
    }
    ImGui::TreePop();
  }

  const auto pipeline = entry.pipeline
                            ? entry.pipeline->get().editablePipeline()
                            : std::nullopt;

  if (pipeline) {
    ImGui::Text("Graphics Pipeline: %s",
                pipeline->get().desc().name.empty()
                    ? "<unnamed>"
                    : pipeline->get().desc().name.c_str());
  } else if (entry.pipeline) {
    ImGui::TextDisabled("Graphics Pipeline: unavailable");
  } else {
    ImGui::TextDisabled("Pipeline: none");
  }

  if (entry.target &&
      ImGui::TreeNodeEx("Render Target", ImGuiTreeNodeFlags_SpanAvailWidth)) {
    const auto &target = entry.target->get().target(0);
    const auto &desc = target.desc();
    ImGui::Text("Extent: %u x %u", target.width(), target.height());
    ImGui::TextDisabled("Attachment configuration, frame 0");
    if (target.hasColor()) {
      ImGui::Text("Color format: %d", static_cast<int>(desc.color.format));
      const bool sampled = (desc.color.usage & VK_IMAGE_USAGE_SAMPLED_BIT) != 0;
      ImGui::Text("Color sampled: %s", sampled ? "yes" : "no");
    } else {
      ImGui::TextDisabled("Color: none");
    }
    if (desc.depth) {
      ImGui::Text("Depth format: %d", static_cast<int>(desc.depth->format));
      const bool retained =
          desc.depth->storeOp == VK_ATTACHMENT_STORE_OP_STORE ||
          (desc.depth->usage & VK_IMAGE_USAGE_SAMPLED_BIT) != 0;
      ImGui::Text("Depth retained: %s", retained ? "yes" : "no");
    } else {
      ImGui::TextDisabled("Depth: none");
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Declared Reads", ImGuiTreeNodeFlags_SpanAvailWidth)) {
    if (pass.reads().empty()) {
      ImGui::TextDisabled("None");
    } else {
      for (const auto &resource : pass.reads()) {
        ImGui::BulletText("%s", resource.c_str());
      }
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Declared Writes", ImGuiTreeNodeFlags_SpanAvailWidth)) {
    if (pass.writes().empty()) {
      ImGui::TextDisabled("None");
    } else {
      for (const auto &resource : pass.writes()) {
        ImGui::BulletText("%s", resource.c_str());
      }
    }
    ImGui::TreePop();
  }
}

} // namespace vkr::ui
