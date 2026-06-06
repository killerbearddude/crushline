#include "graph/GraphSerializer.h"

#include "editor/GraphView.h"
#include "graph/GraphDocument.h"
#include "graph/SampleGraph.h"

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>

namespace
{
bool NearlyEqual(float a, float b)
{
    return std::fabs(a - b) <= 0.0001f;
}

bool Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Graph serializer roundtrip failed: " << message << "\n";
        return false;
    }

    return true;
}

bool CheckVec2(Vec2 actual, Vec2 expected, const char* message)
{
    if (!NearlyEqual(actual.x, expected.x) || !NearlyEqual(actual.y, expected.y))
    {
        std::cerr
            << "Graph serializer roundtrip failed: " << message
            << " expected=(" << expected.x << ", " << expected.y << ")"
            << " actual=(" << actual.x << ", " << actual.y << ")\n";
        return false;
    }

    return true;
}

bool CheckPort(const graph::GraphPort& actual, const graph::GraphPort& expected)
{
    return
        Check(actual.id == expected.id, "port id mismatch") &&
        Check(actual.name == expected.name, "port name mismatch") &&
        Check(actual.resource == expected.resource, "port resource mismatch") &&
        Check(actual.direction == expected.direction, "port direction mismatch");
}

bool CheckNode(const graph::GraphNode& actual, const graph::GraphNode& expected)
{
    if (!Check(actual.id == expected.id, "node id mismatch") ||
        !Check(actual.type == expected.type, "node type mismatch") ||
        !Check(actual.name == expected.name, "node name mismatch") ||
        !Check(NearlyEqual(actual.throughput, expected.throughput), "node throughput mismatch") ||
        !Check(NearlyEqual(actual.capacity, expected.capacity), "node capacity mismatch") ||
        !Check(NearlyEqual(actual.efficiency, expected.efficiency), "node efficiency mismatch") ||
        !Check(NearlyEqual(actual.powerUse, expected.powerUse), "node power use mismatch") ||
        !Check(actual.warning == expected.warning, "node warning flag mismatch") ||
        !Check(actual.warningText == expected.warningText, "node warning text mismatch") ||
        !Check(actual.inputs.size() == expected.inputs.size(), "node input port count mismatch") ||
        !Check(actual.outputs.size() == expected.outputs.size(), "node output port count mismatch"))
    {
        return false;
    }

    for (std::size_t i = 0; i < actual.inputs.size(); ++i)
    {
        if (!CheckPort(actual.inputs[i], expected.inputs[i]))
        {
            return false;
        }
    }

    for (std::size_t i = 0; i < actual.outputs.size(); ++i)
    {
        if (!CheckPort(actual.outputs[i], expected.outputs[i]))
        {
            return false;
        }
    }

    return true;
}

bool CheckEdge(const graph::GraphEdge& actual, const graph::GraphEdge& expected)
{
    return
        Check(actual.id == expected.id, "edge id mismatch") &&
        Check(actual.fromNodeId == expected.fromNodeId, "edge from node mismatch") &&
        Check(actual.fromPortId == expected.fromPortId, "edge from port mismatch") &&
        Check(actual.toNodeId == expected.toNodeId, "edge to node mismatch") &&
        Check(actual.toPortId == expected.toPortId, "edge to port mismatch");
}

bool CheckGraph(const graph::GraphDocument& actual, const graph::GraphDocument& expected)
{
    if (!Check(actual.nextNodeId == expected.nextNodeId, "next node id mismatch") ||
        !Check(actual.nextPortId == expected.nextPortId, "next port id mismatch") ||
        !Check(actual.nextEdgeId == expected.nextEdgeId, "next edge id mismatch") ||
        !Check(actual.nodes.size() == expected.nodes.size(), "node count mismatch") ||
        !Check(actual.edges.size() == expected.edges.size(), "edge count mismatch"))
    {
        return false;
    }

    for (std::size_t i = 0; i < actual.nodes.size(); ++i)
    {
        if (!CheckNode(actual.nodes[i], expected.nodes[i]))
        {
            return false;
        }
    }

    for (std::size_t i = 0; i < actual.edges.size(); ++i)
    {
        if (!CheckEdge(actual.edges[i], expected.edges[i]))
        {
            return false;
        }
    }

    return true;
}

bool CheckNodeVisual(const editor::NodeVisual& actual, const editor::NodeVisual& expected)
{
    return
        Check(actual.nodeId == expected.nodeId, "node visual id mismatch") &&
        CheckVec2(actual.position, expected.position, "node visual position mismatch") &&
        CheckVec2(actual.size, expected.size, "node visual size mismatch") &&
        Check(!actual.selected, "loaded node visual selection should be cleared");
}

bool CheckView(const editor::GraphViewState& actual, const editor::GraphViewState& expected)
{
    if (!CheckVec2(actual.cameraOffset, expected.cameraOffset, "camera offset mismatch") ||
        !Check(NearlyEqual(actual.zoom, expected.zoom), "zoom mismatch") ||
        !Check(actual.nodeVisuals.size() == expected.nodeVisuals.size(), "node visual count mismatch"))
    {
        return false;
    }

    for (const auto& [nodeId, expectedVisual] : expected.nodeVisuals)
    {
        const auto actualIt = actual.nodeVisuals.find(nodeId);
        if (!Check(actualIt != actual.nodeVisuals.end(), "missing node visual"))
        {
            return false;
        }

        if (!CheckNodeVisual(actualIt->second, expectedVisual))
        {
            return false;
        }
    }

    return
        Check(actual.selectedNodeId == -1, "loaded selected node should be cleared") &&
        Check(actual.hoveredNodeId == -1, "loaded hovered node should be cleared") &&
        Check(actual.draggingNodeId == -1, "loaded dragging node should be cleared") &&
        Check(actual.hoveredEdgeId == -1, "loaded hovered edge should be cleared") &&
        Check(actual.selectedEdgeId == -1, "loaded selected edge should be cleared") &&
        Check(actual.hoveredPortNodeId == -1, "loaded hovered port node should be cleared") &&
        Check(actual.hoveredPortId == -1, "loaded hovered port should be cleared") &&
        Check(!actual.draggingWire, "loaded dragging wire should be cleared") &&
        Check(actual.wireStartNodeId == -1, "loaded wire start node should be cleared") &&
        Check(actual.wireStartPortId == -1, "loaded wire start port should be cleared") &&
        Check(!actual.panningCanvas, "loaded panning state should be cleared");
}

std::filesystem::path MakeTempPath()
{
    return std::filesystem::temp_directory_path() / "crushline_graph_serializer_roundtrip.json";
}
}

int RunGraphSerializerRoundTripTest()
{
    graph::GraphDocument graph = graph::CreateSampleFactoryGraph();
    editor::GraphViewState view = editor::CreateSampleFactoryGraphView(graph);

    view.cameraOffset = {123.0f, -45.0f};
    view.zoom = 1.35f;
    view.selectedNodeId = 3;
    view.hoveredNodeId = 4;
    view.selectedEdgeId = 2;
    view.draggingWire = true;
    view.wireStartNodeId = 2;
    view.wireStartPortId = 3;
    view.panningCanvas = true;

    if (auto it = view.nodeVisuals.find(3); it != view.nodeVisuals.end())
    {
        it->second.position = {712.0f, 188.0f};
        it->second.size = {156.0f, 82.0f};
        it->second.selected = true;
    }

    const std::filesystem::path path = MakeTempPath();
    std::filesystem::remove(path);

    std::string errorMessage;
    if (!graph::SaveGraphToFile(graph, view, path.string(), &errorMessage))
    {
        std::cerr << "Graph serializer roundtrip failed to save: " << errorMessage << "\n";
        return 1;
    }

    graph::GraphDocument loadedGraph;
    editor::GraphViewState loadedView;
    if (!graph::LoadGraphFromFile(loadedGraph, loadedView, path.string(), &errorMessage))
    {
        std::cerr << "Graph serializer roundtrip failed to load: " << errorMessage << "\n";
        return 1;
    }

    std::filesystem::remove(path);

    if (!CheckGraph(loadedGraph, graph) || !CheckView(loadedView, view))
    {
        return 1;
    }

    std::cout << "Graph serializer roundtrip test passed.\n";
    return 0;
}
