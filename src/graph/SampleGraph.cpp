// Builds the current hand-authored sample graph used by the interactive
// prototype. This remains a legacy dashboard sample until a later patch replaces
// it with a fully recipe-generated Tier 0 production chain.

#include "graph/SampleGraph.h"

#include "graph/GraphDocument.h"

namespace graph
{

GraphDocument CreateSampleFactoryGraph()
{
    GraphDocument graph;

    const int ironOre = AddNode(graph, NodeType::Source, "Iron Ore");
    const int ironOreOut = AddOutputPort(graph, ironOre, "Iron Ore", ResourceType::IronOre);
    if (GraphNode* node = FindNode(graph, ironOre))
    {
        node->throughput = 120.0f;
        node->capacity = 120.0f;
        node->efficiency = 1.0f;
    }

    const int crusher = AddNode(graph, NodeType::Machine, "Crusher");
    const int crusherIn = AddInputPort(graph, crusher, "Iron Ore", ResourceType::IronOre);
    const int crusherOut = AddOutputPort(graph, crusher, "Crushed Ore", ResourceType::CrushedOre);
    if (GraphNode* node = FindNode(graph, crusher))
    {
        node->throughput = 120.0f;
        node->capacity = 120.0f;
        node->efficiency = 0.90f;
        node->powerUse = 120.0f;
    }

    const int washer = AddNode(graph, NodeType::Machine, "Washer");
    const int washerIn = AddInputPort(graph, washer, "Crushed Ore", ResourceType::CrushedOre);
    const int washerOut = AddOutputPort(graph, washer, "Washed Ore", ResourceType::WashedOre);
    const int washerWaste = AddOutputPort(graph, washer, "Slurry", ResourceType::Slurry);
    if (GraphNode* node = FindNode(graph, washer))
    {
        node->throughput = 108.0f;
        node->capacity = 120.0f;
        node->efficiency = 0.90f;
        node->powerUse = 120.0f;
        node->warning = true;
        node->warningText = "Clogged by high slurry content";
    }

    const int furnace = AddNode(graph, NodeType::Machine, "Furnace");
    const int furnaceIn = AddInputPort(graph, furnace, "Washed Ore", ResourceType::WashedOre);
    const int furnaceOut = AddOutputPort(graph, furnace, "Iron Ingot", ResourceType::IronIngot);
    if (GraphNode* node = FindNode(graph, furnace))
    {
        node->throughput = 92.0f;
        node->capacity = 100.0f;
        node->efficiency = 0.95f;
        node->powerUse = 300.0f;
    }

    const int ironIngot = AddNode(graph, NodeType::Output, "Iron Ingot");
    const int ironIngotIn = AddInputPort(graph, ironIngot, "Iron Ingot", ResourceType::IronIngot);
    if (GraphNode* node = FindNode(graph, ironIngot))
    {
        node->throughput = 88.0f;
        node->capacity = 100.0f;
        node->efficiency = 0.88f;
    }

    const int slurry = AddNode(graph, NodeType::Storage, "Slurry");
    const int slurryIn = AddInputPort(graph, slurry, "Slurry", ResourceType::Slurry);
    const int slurryOut = AddOutputPort(graph, slurry, "Slurry", ResourceType::Slurry);
    if (GraphNode* node = FindNode(graph, slurry))
    {
        node->throughput = 12.0f;
        node->capacity = 24.0f;
        node->efficiency = 1.0f;
    }

    const int wasteStorage = AddNode(graph, NodeType::Storage, "Waste Storage");
    const int wasteIn = AddInputPort(graph, wasteStorage, "Slurry", ResourceType::Slurry);
    if (GraphNode* node = FindNode(graph, wasteStorage))
    {
        node->throughput = 23.0f;
        node->capacity = 25.0f;
        node->efficiency = 0.92f;
        node->warning = true;
        node->warningText = "Waste capacity above 90%";
    }

    (void)AddEdge(graph, ironOre, ironOreOut, crusher, crusherIn);
    (void)AddEdge(graph, crusher, crusherOut, washer, washerIn);
    (void)AddEdge(graph, washer, washerOut, furnace, furnaceIn);
    (void)AddEdge(graph, furnace, furnaceOut, ironIngot, ironIngotIn);
    (void)AddEdge(graph, washer, washerWaste, slurry, slurryIn);
    (void)AddEdge(graph, slurry, slurryOut, wasteStorage, wasteIn);

    return graph;
}

} // namespace graph
