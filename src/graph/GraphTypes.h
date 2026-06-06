#pragma once

#include <string>
#include <vector>

namespace graph
{

enum class ResourceType
{
    IronOre,
    CrushedOre,
    WashedOre,
    Slurry,
    Tailings,
    IronIngot,
    CopperOre,
    CopperCathode,
    Coal,
    ScrapMetal,
    Stone,
    Concrete,
    Waste,
    Power,
    Water
};

enum class NodeType
{
    Source,
    Machine,
    Storage,
    Output,
    Contract,
    Modifier,
    Warning
};

enum class PortDirection
{
    Input,
    Output
};

struct GraphPort
{
    int id = 0;
    std::string name{};
    ResourceType resource = ResourceType::IronOre;
    PortDirection direction = PortDirection::Input;
};

struct GraphNode
{
    int id = 0;
    NodeType type = NodeType::Machine;
    std::string name{};

    std::vector<GraphPort> inputs{};
    std::vector<GraphPort> outputs{};

    float throughput = 0.0f;
    float capacity = 0.0f;
    float efficiency = 1.0f;
    float powerUse = 0.0f;

    bool warning = false;
    std::string warningText{};
};

struct GraphEdge
{
    int id = 0;

    int fromNodeId = 0;
    int fromPortId = 0;

    int toNodeId = 0;
    int toPortId = 0;
};

struct GraphDocument
{
    std::vector<GraphNode> nodes{};
    std::vector<GraphEdge> edges{};

    int nextNodeId = 1;
    int nextPortId = 1;
    int nextEdgeId = 1;
};

} // namespace graph
