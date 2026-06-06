// Builds the interactive sample graph shown when Crushline starts. The sample
// now uses Tier 0 production catalog recipes for all machine/process nodes while
// keeping one temporary legacy target node until scenario objectives own targets.

#include "graph/SampleGraph.h"

#include "graph/GraphDocument.h"

namespace graph
{
namespace
{
const GraphPort* FindInputByProductionResource(const GraphNode& node, ResourceId resourceId)
{
    for (const GraphPort& port : node.inputs)
    {
        if (port.productionResourceId == resourceId)
        {
            return &port;
        }
    }

    return nullptr;
}

const GraphPort* FindOutputByProductionResource(const GraphNode& node, ResourceId resourceId)
{
    for (const GraphPort& port : node.outputs)
    {
        if (port.productionResourceId == resourceId)
        {
            return &port;
        }
    }

    return nullptr;
}

void RenameNode(GraphDocument& graph, int nodeId, const char* name)
{
    if (GraphNode* node = FindNode(graph, nodeId))
    {
        node->name = name;
    }
}

void SetLegacyDashboardMetrics(
    GraphDocument& graph,
    int nodeId,
    float throughput,
    float capacity,
    float efficiency
)
{
    // TEMP: The current dashboard still reads legacy throughput/capacity fields.
    // Recipe-rate evaluation will replace these values when the production
    // evaluator becomes the source of truth.
    if (GraphNode* node = FindNode(graph, nodeId))
    {
        node->throughput = throughput;
        node->capacity = capacity;
        node->efficiency = efficiency;
    }
}

bool ConnectByProductionResource(
    GraphDocument& graph,
    int fromNodeId,
    ResourceId fromResourceId,
    int toNodeId,
    ResourceId toResourceId
)
{
    const GraphNode* fromNode = FindNode(graph, fromNodeId);
    const GraphNode* toNode = FindNode(graph, toNodeId);

    if (fromNode == nullptr || toNode == nullptr)
    {
        return false;
    }

    const GraphPort* fromPort = FindOutputByProductionResource(*fromNode, fromResourceId);
    const GraphPort* toPort = FindInputByProductionResource(*toNode, toResourceId);

    if (fromPort == nullptr || toPort == nullptr)
    {
        return false;
    }

    return AddEdge(graph, fromNodeId, fromPort->id, toNodeId, toPort->id) != 0;
}
} // namespace

GraphDocument CreateSampleFactoryGraph()
{
    const ResourceCatalog resources;
    const MachineCatalog machines;
    const RecipeCatalog recipes;

    GraphDocument graph;

    const int ironOreSource = AddRecipeNode(
        graph,
        production_ids::ResourceSource,
        production_ids::ExtractIronOre,
        machines,
        recipes,
        resources
    );

    const int waterSource = AddRecipeNode(
        graph,
        production_ids::ResourceSource,
        production_ids::SupplyWater,
        machines,
        recipes,
        resources
    );

    const int crusher = AddRecipeNode(
        graph,
        production_ids::Crusher,
        production_ids::CrushIronOre,
        machines,
        recipes,
        resources
    );

    const int washer = AddRecipeNode(
        graph,
        production_ids::Washer,
        production_ids::WashCrushedIronOre,
        machines,
        recipes,
        resources
    );

    const int smelter = AddRecipeNode(
        graph,
        production_ids::Smelter,
        production_ids::SmeltWashedIronOre,
        machines,
        recipes,
        resources
    );

    const int wasteSink = AddRecipeNode(
        graph,
        production_ids::WasteSink,
        production_ids::StoreIronSlurry,
        machines,
        recipes,
        resources
    );

    // The objective system is not implemented yet, so the sample keeps a legacy
    // output node as a visible endpoint for the Iron Ingot chain and old metrics.
    const int ironIngotTarget = AddNode(graph, NodeType::Output, "Iron Ingot Target");
    const int ironIngotInput = AddInputPort(
        graph,
        ironIngotTarget,
        "Iron Ingot",
        ResourceType::IronIngot,
        production_ids::IronIngot
    );
    (void)ironIngotInput;

    RenameNode(graph, ironOreSource, "Iron Ore Source");
    RenameNode(graph, waterSource, "Water Source");

    SetLegacyDashboardMetrics(graph, ironOreSource, 60.0f, 60.0f, 1.0f);
    SetLegacyDashboardMetrics(graph, waterSource, 60.0f, 60.0f, 1.0f);
    SetLegacyDashboardMetrics(graph, crusher, 60.0f, 60.0f, 1.0f);
    SetLegacyDashboardMetrics(graph, washer, 50.0f, 60.0f, 1.0f);
    SetLegacyDashboardMetrics(graph, smelter, 50.0f, 50.0f, 1.0f);
    SetLegacyDashboardMetrics(graph, wasteSink, 10.0f, 10.0f, 1.0f);
    SetLegacyDashboardMetrics(graph, ironIngotTarget, 50.0f, 50.0f, 1.0f);

    (void)ConnectByProductionResource(
        graph,
        ironOreSource,
        production_ids::IronOre,
        crusher,
        production_ids::IronOre
    );
    (void)ConnectByProductionResource(
        graph,
        crusher,
        production_ids::CrushedIronOre,
        washer,
        production_ids::CrushedIronOre
    );
    (void)ConnectByProductionResource(
        graph,
        waterSource,
        production_ids::Water,
        washer,
        production_ids::Water
    );
    (void)ConnectByProductionResource(
        graph,
        washer,
        production_ids::WashedIronOre,
        smelter,
        production_ids::WashedIronOre
    );
    (void)ConnectByProductionResource(
        graph,
        smelter,
        production_ids::IronIngot,
        ironIngotTarget,
        production_ids::IronIngot
    );
    (void)ConnectByProductionResource(
        graph,
        washer,
        production_ids::IronSlurry,
        wasteSink,
        production_ids::IronSlurry
    );

    return graph;
}

} // namespace graph
