#pragma once

// Defines the editable graph document data used by the node editor, serializer,
// evaluator, and production-catalog integration. The graph keeps legacy resource
// enums for existing UI/evaluator code while also storing production catalog IDs
// so recipe-driven nodes can become the authoritative gameplay path.

#include "graph/ProductionTypes.h"

#include <string>
#include <vector>

namespace graph
{

// Legacy visual/evaluator resource enum. Production catalog ResourceId values
// are stored on ports as well and should become the long-term source of truth.
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

// Broad editor node category used for styling and current dashboard summaries.
// Recipe-driven machine identity is stored separately through machineId/recipeId.
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

// Port direction is part of connection validation. MVP graph rules allow only
// output-to-input links.
enum class PortDirection
{
    Input,
    Output
};

// A graph port represents one connectable recipe input or output. The legacy
// ResourceType keeps existing rendering/evaluator behavior stable; productionId
// stores the catalog ResourceId used by recipe-generated ports.
struct GraphPort
{
    int id = 0;
    std::string name{};
    ResourceType resource = ResourceType::IronOre;
    PortDirection direction = PortDirection::Input;
    ResourceId productionResourceId = InvalidResourceId;
    bool isByproduct = false;
};

// A graph node is an editor-owned machine/process instance. Legacy metric fields
// remain for the current dashboard while machineId/recipeId attach catalog data
// for the upcoming production evaluator.
struct GraphNode
{
    int id = 0;
    NodeType type = NodeType::Machine;
    std::string name{};

    MachineId machineId = InvalidMachineId;
    RecipeId recipeId = InvalidRecipeId;

    std::vector<GraphPort> inputs{};
    std::vector<GraphPort> outputs{};

    float throughput = 0.0f;
    float capacity = 0.0f;
    float efficiency = 1.0f;
    float powerUse = 0.0f;

    bool warning = false;
    std::string warningText{};
};

// Directed connection between one output port and one input port.
struct GraphEdge
{
    int id = 0;

    int fromNodeId = 0;
    int fromPortId = 0;

    int toNodeId = 0;
    int toPortId = 0;
};

// Mutable graph document. ID counters are monotonic so undo/redo and save/load
// can avoid accidentally reusing identifiers from deleted nodes, ports, or edges.
struct GraphDocument
{
    std::vector<GraphNode> nodes{};
    std::vector<GraphEdge> edges{};

    int nextNodeId = 1;
    int nextPortId = 1;
    int nextEdgeId = 1;
};

} // namespace graph
