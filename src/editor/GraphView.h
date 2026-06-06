#pragma once

#include "core/Math.h"
#include "core/Rect.h"
#include "graph/GraphTypes.h"
#include "ui/Theme.h"

#include <unordered_map>

class Renderer2D;

namespace editor
{

struct NodeVisual
{
    int nodeId = 0;
    Vec2 position{};
    Vec2 size = {128.0f, 64.0f};
    bool selected = false;
};

struct GraphViewState
{
    Vec2 cameraOffset = {0.0f, 0.0f};
    float zoom = 1.0f;

    std::unordered_map<int, NodeVisual> nodeVisuals{};

    int selectedNodeId = -1;
    int hoveredNodeId = -1;
    int draggingNodeId = -1;
};

GraphViewState CreateSampleFactoryGraphView(const graph::GraphDocument& graph);
void EnsureNodeVisuals(GraphViewState& view, const graph::GraphDocument& graph);
void DrawGraphView(
    Renderer2D& renderer,
    const graph::GraphDocument& graph,
    const GraphViewState& view,
    Rect canvasRect,
    const UiTheme& theme
);

} // namespace editor
