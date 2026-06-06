#include "graph/GraphSerializer.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace graph
{
namespace
{
using Json = nlohmann::json;

const char* ToString(ResourceType value)
{
    switch (value)
    {
        case ResourceType::IronOre: return "IronOre";
        case ResourceType::CrushedOre: return "CrushedOre";
        case ResourceType::WashedOre: return "WashedOre";
        case ResourceType::Slurry: return "Slurry";
        case ResourceType::Tailings: return "Tailings";
        case ResourceType::IronIngot: return "IronIngot";
        case ResourceType::CopperOre: return "CopperOre";
        case ResourceType::CopperCathode: return "CopperCathode";
        case ResourceType::Coal: return "Coal";
        case ResourceType::ScrapMetal: return "ScrapMetal";
        case ResourceType::Stone: return "Stone";
        case ResourceType::Concrete: return "Concrete";
        case ResourceType::Waste: return "Waste";
        case ResourceType::Power: return "Power";
        case ResourceType::Water: return "Water";
    }

    return "IronOre";
}

const char* ToString(NodeType value)
{
    switch (value)
    {
        case NodeType::Source: return "Source";
        case NodeType::Machine: return "Machine";
        case NodeType::Storage: return "Storage";
        case NodeType::Output: return "Output";
        case NodeType::Contract: return "Contract";
        case NodeType::Modifier: return "Modifier";
        case NodeType::Warning: return "Warning";
    }

    return "Machine";
}

const char* ToString(PortDirection value)
{
    switch (value)
    {
        case PortDirection::Input: return "Input";
        case PortDirection::Output: return "Output";
    }

    return "Input";
}

ResourceType ParseResourceType(const std::string& value)
{
    if (value == "IronOre") return ResourceType::IronOre;
    if (value == "CrushedOre") return ResourceType::CrushedOre;
    if (value == "WashedOre") return ResourceType::WashedOre;
    if (value == "Slurry") return ResourceType::Slurry;
    if (value == "Tailings") return ResourceType::Tailings;
    if (value == "IronIngot") return ResourceType::IronIngot;
    if (value == "CopperOre") return ResourceType::CopperOre;
    if (value == "CopperCathode") return ResourceType::CopperCathode;
    if (value == "Coal") return ResourceType::Coal;
    if (value == "ScrapMetal") return ResourceType::ScrapMetal;
    if (value == "Stone") return ResourceType::Stone;
    if (value == "Concrete") return ResourceType::Concrete;
    if (value == "Waste") return ResourceType::Waste;
    if (value == "Power") return ResourceType::Power;
    if (value == "Water") return ResourceType::Water;

    throw std::runtime_error("Unknown resource type: " + value);
}

NodeType ParseNodeType(const std::string& value)
{
    if (value == "Source") return NodeType::Source;
    if (value == "Machine") return NodeType::Machine;
    if (value == "Storage") return NodeType::Storage;
    if (value == "Output") return NodeType::Output;
    if (value == "Contract") return NodeType::Contract;
    if (value == "Modifier") return NodeType::Modifier;
    if (value == "Warning") return NodeType::Warning;

    throw std::runtime_error("Unknown node type: " + value);
}

PortDirection ParsePortDirection(const std::string& value)
{
    if (value == "Input") return PortDirection::Input;
    if (value == "Output") return PortDirection::Output;

    throw std::runtime_error("Unknown port direction: " + value);
}

Json SerializeVec2(Vec2 value)
{
    return Json{
        {"x", value.x},
        {"y", value.y}
    };
}

Vec2 DeserializeVec2(const Json& value)
{
    return Vec2{
        value.at("x").get<float>(),
        value.at("y").get<float>()
    };
}

Json SerializePort(const GraphPort& port)
{
    return Json{
        {"id", port.id},
        {"name", port.name},
        {"resource", ToString(port.resource)},
        {"direction", ToString(port.direction)}
    };
}

GraphPort DeserializePort(const Json& value)
{
    return GraphPort{
        .id = value.at("id").get<int>(),
        .name = value.at("name").get<std::string>(),
        .resource = ParseResourceType(value.at("resource").get<std::string>()),
        .direction = ParsePortDirection(value.at("direction").get<std::string>())
    };
}

Json SerializeNode(const GraphNode& node)
{
    Json inputs = Json::array();
    for (const GraphPort& port : node.inputs)
    {
        inputs.push_back(SerializePort(port));
    }

    Json outputs = Json::array();
    for (const GraphPort& port : node.outputs)
    {
        outputs.push_back(SerializePort(port));
    }

    return Json{
        {"id", node.id},
        {"type", ToString(node.type)},
        {"name", node.name},
        {"throughput", node.throughput},
        {"capacity", node.capacity},
        {"efficiency", node.efficiency},
        {"powerUse", node.powerUse},
        {"warning", node.warning},
        {"warningText", node.warningText},
        {"inputs", std::move(inputs)},
        {"outputs", std::move(outputs)}
    };
}

GraphNode DeserializeNode(const Json& value)
{
    GraphNode node{
        .id = value.at("id").get<int>(),
        .type = ParseNodeType(value.at("type").get<std::string>()),
        .name = value.at("name").get<std::string>(),
        .throughput = value.value("throughput", 0.0f),
        .capacity = value.value("capacity", 0.0f),
        .efficiency = value.value("efficiency", 1.0f),
        .powerUse = value.value("powerUse", 0.0f),
        .warning = value.value("warning", false),
        .warningText = value.value("warningText", std::string{})
    };

    for (const Json& port : value.value("inputs", Json::array()))
    {
        node.inputs.push_back(DeserializePort(port));
    }

    for (const Json& port : value.value("outputs", Json::array()))
    {
        node.outputs.push_back(DeserializePort(port));
    }

    return node;
}

Json SerializeEdge(const GraphEdge& edge)
{
    return Json{
        {"id", edge.id},
        {"fromNodeId", edge.fromNodeId},
        {"fromPortId", edge.fromPortId},
        {"toNodeId", edge.toNodeId},
        {"toPortId", edge.toPortId}
    };
}

GraphEdge DeserializeEdge(const Json& value)
{
    return GraphEdge{
        .id = value.at("id").get<int>(),
        .fromNodeId = value.at("fromNodeId").get<int>(),
        .fromPortId = value.at("fromPortId").get<int>(),
        .toNodeId = value.at("toNodeId").get<int>(),
        .toPortId = value.at("toPortId").get<int>()
    };
}

Json SerializeNodeVisual(const editor::NodeVisual& visual)
{
    return Json{
        {"nodeId", visual.nodeId},
        {"position", SerializeVec2(visual.position)},
        {"size", SerializeVec2(visual.size)},
        {"selected", visual.selected}
    };
}

editor::NodeVisual DeserializeNodeVisual(const Json& value)
{
    return editor::NodeVisual{
        .nodeId = value.at("nodeId").get<int>(),
        .position = DeserializeVec2(value.at("position")),
        .size = DeserializeVec2(value.at("size")),
        .selected = value.value("selected", false)
    };
}

int MaxNodeId(const GraphDocument& graph)
{
    int maxId = 0;
    for (const GraphNode& node : graph.nodes)
    {
        maxId = std::max(maxId, node.id);
    }
    return maxId;
}

int MaxPortId(const GraphDocument& graph)
{
    int maxId = 0;
    for (const GraphNode& node : graph.nodes)
    {
        for (const GraphPort& port : node.inputs)
        {
            maxId = std::max(maxId, port.id);
        }
        for (const GraphPort& port : node.outputs)
        {
            maxId = std::max(maxId, port.id);
        }
    }
    return maxId;
}

int MaxEdgeId(const GraphDocument& graph)
{
    int maxId = 0;
    for (const GraphEdge& edge : graph.edges)
    {
        maxId = std::max(maxId, edge.id);
    }
    return maxId;
}

bool HasNodeId(const GraphDocument& graph, int nodeId)
{
    if (nodeId < 0)
    {
        return false;
    }

    return std::any_of(
        graph.nodes.begin(),
        graph.nodes.end(),
        [nodeId](const GraphNode& node)
        {
            return node.id == nodeId;
        }
    );
}

bool HasEdgeId(const GraphDocument& graph, int edgeId)
{
    if (edgeId < 0)
    {
        return false;
    }

    return std::any_of(
        graph.edges.begin(),
        graph.edges.end(),
        [edgeId](const GraphEdge& edge)
        {
            return edge.id == edgeId;
        }
    );
}

void SetError(std::string* errorMessage, const std::string& message)
{
    if (errorMessage != nullptr)
    {
        *errorMessage = message;
    }
}
}

bool SaveGraphToFile(
    const GraphDocument& graph,
    const editor::GraphViewState& view,
    const std::string& path,
    std::string* errorMessage
)
{
    try
    {
        const std::filesystem::path filePath{path};
        const std::filesystem::path parent = filePath.parent_path();
        if (!parent.empty())
        {
            std::filesystem::create_directories(parent);
        }

        Json nodes = Json::array();
        for (const GraphNode& node : graph.nodes)
        {
            nodes.push_back(SerializeNode(node));
        }

        Json edges = Json::array();
        for (const GraphEdge& edge : graph.edges)
        {
            edges.push_back(SerializeEdge(edge));
        }

        Json nodeVisuals = Json::array();
        for (const auto& [nodeId, visual] : view.nodeVisuals)
        {
            (void)nodeId;
            nodeVisuals.push_back(SerializeNodeVisual(visual));
        }

        Json root{
            {"version", 1},
            {"graph", Json{
                {"nextNodeId", graph.nextNodeId},
                {"nextPortId", graph.nextPortId},
                {"nextEdgeId", graph.nextEdgeId},
                {"nodes", std::move(nodes)},
                {"edges", std::move(edges)}
            }},
            {"view", Json{
                {"cameraOffset", SerializeVec2(view.cameraOffset)},
                {"zoom", view.zoom},
                {"selectedNodeId", view.selectedNodeId},
                {"selectedEdgeId", view.selectedEdgeId},
                {"nodeVisuals", std::move(nodeVisuals)}
            }}
        };

        std::ofstream output{filePath};
        if (!output)
        {
            SetError(errorMessage, "Unable to open graph file for writing: " + path);
            return false;
        }

        output << root.dump(4) << '\n';
        return true;
    }
    catch (const std::exception& ex)
    {
        SetError(errorMessage, ex.what());
        return false;
    }
}

bool LoadGraphFromFile(
    GraphDocument& graph,
    editor::GraphViewState& view,
    const std::string& path,
    std::string* errorMessage
)
{
    try
    {
        std::ifstream input{path};
        if (!input)
        {
            SetError(errorMessage, "Unable to open graph file for reading: " + path);
            return false;
        }

        Json root;
        input >> root;

        if (root.value("version", 0) != 1)
        {
            SetError(errorMessage, "Unsupported graph file version.");
            return false;
        }

        const Json& graphJson = root.at("graph");

        GraphDocument loadedGraph{};
        for (const Json& node : graphJson.value("nodes", Json::array()))
        {
            loadedGraph.nodes.push_back(DeserializeNode(node));
        }

        for (const Json& edge : graphJson.value("edges", Json::array()))
        {
            loadedGraph.edges.push_back(DeserializeEdge(edge));
        }

        loadedGraph.nextNodeId = graphJson.value("nextNodeId", MaxNodeId(loadedGraph) + 1);
        loadedGraph.nextPortId = graphJson.value("nextPortId", MaxPortId(loadedGraph) + 1);
        loadedGraph.nextEdgeId = graphJson.value("nextEdgeId", MaxEdgeId(loadedGraph) + 1);

        editor::GraphViewState loadedView{};
        if (root.contains("view"))
        {
            const Json& viewJson = root.at("view");
            loadedView.cameraOffset = DeserializeVec2(viewJson.value("cameraOffset", Json{{"x", 0.0f}, {"y", 0.0f}}));
            loadedView.zoom = viewJson.value("zoom", 1.0f);

            const int selectedNodeId = viewJson.value("selectedNodeId", -1);
            const int selectedEdgeId = viewJson.value("selectedEdgeId", -1);

            loadedView.selectedNodeId = HasNodeId(loadedGraph, selectedNodeId)
                ? selectedNodeId
                : -1;

            loadedView.selectedEdgeId = HasEdgeId(loadedGraph, selectedEdgeId)
                ? selectedEdgeId
                : -1;

            for (const Json& visual : viewJson.value("nodeVisuals", Json::array()))
            {
                editor::NodeVisual nodeVisual = DeserializeNodeVisual(visual);
                loadedView.nodeVisuals[nodeVisual.nodeId] = nodeVisual;
            }
        }

        loadedView.hoveredNodeId = -1;
        loadedView.draggingNodeId = -1;
        loadedView.hoveredEdgeId = -1;
        loadedView.hoveredPortNodeId = -1;
        loadedView.hoveredPortId = -1;
        loadedView.draggingWire = false;
        loadedView.wireStartNodeId = -1;
        loadedView.wireStartPortId = -1;
        loadedView.lastWireDropFailure = editor::WireDropFailureReason::None;
        loadedView.panningCanvas = false;

        graph = std::move(loadedGraph);
        view = std::move(loadedView);
        editor::EnsureNodeVisuals(view, graph);
        return true;
    }
    catch (const std::exception& ex)
    {
        SetError(errorMessage, ex.what());
        return false;
    }
}

} // namespace graph
