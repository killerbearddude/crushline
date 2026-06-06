// Verifies that graph nodes can be configured from production catalog machine
// and recipe definitions. These tests prevent regressions where generated ports
// lose production ResourceId data or invalid machine/recipe pairings mutate the
// graph before the production evaluator starts depending on this model.

#include "graph/GraphDocument.h"
#include "graph/SampleGraph.h"

#include <cstddef>
#include <iostream>

namespace
{
bool Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Graph recipe-node test failed: " << message << "\n";
        return false;
    }

    return true;
}

struct CatalogSet
{
    graph::ResourceCatalog resources;
    graph::MachineCatalog machines;
    graph::RecipeCatalog recipes;
};

const graph::GraphNode* RequireNode(const graph::GraphDocument& graph, int nodeId)
{
    const graph::GraphNode* node = graph::FindNode(graph, nodeId);
    Check(node != nullptr, "required node should exist");
    return node;
}

const graph::GraphPort* FindInputByProductionResource(
    const graph::GraphNode& node,
    graph::ResourceId resourceId
)
{
    for (const graph::GraphPort& port : node.inputs)
    {
        if (port.productionResourceId == resourceId)
        {
            return &port;
        }
    }

    return nullptr;
}

const graph::GraphPort* FindOutputByProductionResource(
    const graph::GraphNode& node,
    graph::ResourceId resourceId
)
{
    for (const graph::GraphPort& port : node.outputs)
    {
        if (port.productionResourceId == resourceId)
        {
            return &port;
        }
    }

    return nullptr;
}


const graph::GraphNode* FindNodeByRecipe(
    const graph::GraphDocument& graph,
    graph::MachineId machineId,
    graph::RecipeId recipeId
)
{
    for (const graph::GraphNode& node : graph.nodes)
    {
        if (node.machineId == machineId && node.recipeId == recipeId)
        {
            return &node;
        }
    }

    return nullptr;
}

const graph::GraphNode* FindOutputNodeForResource(
    const graph::GraphDocument& graph,
    graph::ResourceId resourceId
)
{
    for (const graph::GraphNode& node : graph.nodes)
    {
        if (node.type != graph::NodeType::Output)
        {
            continue;
        }

        if (FindInputByProductionResource(node, resourceId) != nullptr)
        {
            return &node;
        }
    }

    return nullptr;
}

bool HasEdge(
    const graph::GraphDocument& graph,
    int fromNodeId,
    graph::ResourceId fromResourceId,
    int toNodeId,
    graph::ResourceId toResourceId
)
{
    const graph::GraphNode* fromNode = graph::FindNode(graph, fromNodeId);
    const graph::GraphNode* toNode = graph::FindNode(graph, toNodeId);

    if (fromNode == nullptr || toNode == nullptr)
    {
        return false;
    }

    const graph::GraphPort* fromPort = FindOutputByProductionResource(*fromNode, fromResourceId);
    const graph::GraphPort* toPort = FindInputByProductionResource(*toNode, toResourceId);

    if (fromPort == nullptr || toPort == nullptr)
    {
        return false;
    }

    for (const graph::GraphEdge& edge : graph.edges)
    {
        if (edge.fromNodeId == fromNodeId &&
            edge.fromPortId == fromPort->id &&
            edge.toNodeId == toNodeId &&
            edge.toPortId == toPort->id)
        {
            return true;
        }
    }

    return false;
}

bool CheckGeneratedPort(
    const graph::GraphPort* port,
    graph::PortDirection expectedDirection,
    graph::ResourceId expectedProductionResourceId,
    graph::ResourceType expectedLegacyResource,
    bool expectedByproduct,
    const char* missingMessage
)
{
    if (!Check(port != nullptr, missingMessage))
    {
        return false;
    }

    return
        Check(port->direction == expectedDirection, "generated port direction mismatch") &&
        Check(port->productionResourceId == expectedProductionResourceId, "generated port production resource mismatch") &&
        Check(port->resource == expectedLegacyResource, "generated port legacy resource mismatch") &&
        Check(port->isByproduct == expectedByproduct, "generated port byproduct flag mismatch");
}

bool CheckCrusherRecipeNode()
{
    // Verifies the simplest recipe-driven machine: one input and one output.
    // This protects the contract that recipe inputs/outputs become graph ports.
    const CatalogSet catalogs;
    graph::GraphDocument graph;

    const int nodeId = graph::AddRecipeNode(
        graph,
        graph::production_ids::Crusher,
        graph::production_ids::CrushIronOre,
        catalogs.machines,
        catalogs.recipes,
        catalogs.resources
    );

    const graph::GraphNode* node = RequireNode(graph, nodeId);
    if (!Check(nodeId != 0, "crusher recipe node should be created") ||
        !Check(node != nullptr, "crusher node should be findable"))
    {
        return false;
    }

    const graph::GraphPort* ironInput = FindInputByProductionResource(*node, graph::production_ids::IronOre);
    const graph::GraphPort* crushedOutput = FindOutputByProductionResource(*node, graph::production_ids::CrushedIronOre);

    return
        Check(graph.nodes.size() == 1, "crusher graph should contain one node") &&
        Check(node->type == graph::NodeType::Machine, "crusher should be a machine node") &&
        Check(node->name == "Crusher", "crusher node should use machine display name") &&
        Check(node->machineId == graph::production_ids::Crusher, "crusher machine id mismatch") &&
        Check(node->recipeId == graph::production_ids::CrushIronOre, "crusher recipe id mismatch") &&
        Check(node->inputs.size() == 1, "crusher should have one generated input") &&
        Check(node->outputs.size() == 1, "crusher should have one generated output") &&
        CheckGeneratedPort(
            ironInput,
            graph::PortDirection::Input,
            graph::production_ids::IronOre,
            graph::ResourceType::IronOre,
            false,
            "crusher iron input should exist"
        ) &&
        CheckGeneratedPort(
            crushedOutput,
            graph::PortDirection::Output,
            graph::production_ids::CrushedIronOre,
            graph::ResourceType::CrushedOre,
            false,
            "crusher crushed ore output should exist"
        );
}

bool CheckWasherRecipeNodeWithByproduct()
{
    // Verifies multi-input recipes and byproduct outputs. The Iron Slurry output
    // must be preserved as an output port and tagged so objective rules can later
    // require it to be handled by a sink.
    const CatalogSet catalogs;
    graph::GraphDocument graph;

    const int nodeId = graph::AddRecipeNode(
        graph,
        graph::production_ids::Washer,
        graph::production_ids::WashCrushedIronOre,
        catalogs.machines,
        catalogs.recipes,
        catalogs.resources
    );

    const graph::GraphNode* node = RequireNode(graph, nodeId);
    if (!Check(nodeId != 0, "washer recipe node should be created") ||
        !Check(node != nullptr, "washer node should be findable"))
    {
        return false;
    }

    const graph::GraphPort* crushedInput = FindInputByProductionResource(*node, graph::production_ids::CrushedIronOre);
    const graph::GraphPort* waterInput = FindInputByProductionResource(*node, graph::production_ids::Water);
    const graph::GraphPort* washedOutput = FindOutputByProductionResource(*node, graph::production_ids::WashedIronOre);
    const graph::GraphPort* slurryOutput = FindOutputByProductionResource(*node, graph::production_ids::IronSlurry);

    return
        Check(node->inputs.size() == 2, "washer should have two generated inputs") &&
        Check(node->outputs.size() == 2, "washer should have one output and one byproduct") &&
        CheckGeneratedPort(
            crushedInput,
            graph::PortDirection::Input,
            graph::production_ids::CrushedIronOre,
            graph::ResourceType::CrushedOre,
            false,
            "washer crushed ore input should exist"
        ) &&
        CheckGeneratedPort(
            waterInput,
            graph::PortDirection::Input,
            graph::production_ids::Water,
            graph::ResourceType::Water,
            false,
            "washer water input should exist"
        ) &&
        CheckGeneratedPort(
            washedOutput,
            graph::PortDirection::Output,
            graph::production_ids::WashedIronOre,
            graph::ResourceType::WashedOre,
            false,
            "washer washed ore output should exist"
        ) &&
        CheckGeneratedPort(
            slurryOutput,
            graph::PortDirection::Output,
            graph::production_ids::IronSlurry,
            graph::ResourceType::Slurry,
            true,
            "washer slurry byproduct output should exist"
        );
}

bool CheckInvalidMachineRecipePairIsRejected()
{
    // Verifies that compatibility is checked before mutation. This prevents a
    // bad Add Node choice from consuming graph IDs or leaving an invalid node.
    const CatalogSet catalogs;
    graph::GraphDocument graph;

    const int nodeId = graph::AddRecipeNode(
        graph,
        graph::production_ids::Crusher,
        graph::production_ids::WashCrushedIronOre,
        catalogs.machines,
        catalogs.recipes,
        catalogs.resources
    );

    return
        Check(nodeId == 0, "crusher should not be allowed to run washer recipe") &&
        Check(graph.nodes.empty(), "invalid recipe node should not append a node") &&
        Check(graph.nextNodeId == 1, "invalid recipe node should not consume node id") &&
        Check(graph.nextPortId == 1, "invalid recipe node should not consume port ids");
}

bool CheckReconfigureRemovesStaleEdges()
{
    // Reconfiguration rebuilds port IDs. Existing edges touching that node must
    // be removed so the document cannot retain dangling endpoints.
    const CatalogSet catalogs;
    graph::GraphDocument graph;

    const int sourceId = graph::AddRecipeNode(
        graph,
        graph::production_ids::ResourceSource,
        graph::production_ids::ExtractIronOre,
        catalogs.machines,
        catalogs.recipes,
        catalogs.resources
    );

    const int crusherId = graph::AddRecipeNode(
        graph,
        graph::production_ids::Crusher,
        graph::production_ids::CrushIronOre,
        catalogs.machines,
        catalogs.recipes,
        catalogs.resources
    );

    const graph::GraphNode* source = RequireNode(graph, sourceId);
    const graph::GraphNode* crusher = RequireNode(graph, crusherId);
    if (!Check(source != nullptr, "source should exist before reconfigure") ||
        !Check(crusher != nullptr, "crusher should exist before reconfigure"))
    {
        return false;
    }

    const graph::GraphPort* sourceIron = FindOutputByProductionResource(*source, graph::production_ids::IronOre);
    const graph::GraphPort* crusherIron = FindInputByProductionResource(*crusher, graph::production_ids::IronOre);
    if (!Check(sourceIron != nullptr, "source iron output should exist before reconfigure") ||
        !Check(crusherIron != nullptr, "crusher iron input should exist before reconfigure"))
    {
        return false;
    }

    const int edgeId = graph::AddEdge(graph, sourceId, sourceIron->id, crusherId, crusherIron->id);
    if (!Check(edgeId != 0, "initial recipe-generated edge should be valid"))
    {
        return false;
    }

    const bool reconfigured = graph::ConfigureNodeFromRecipe(
        graph,
        sourceId,
        graph::production_ids::ResourceSource,
        graph::production_ids::SupplyWater,
        catalogs.machines,
        catalogs.recipes,
        catalogs.resources
    );

    const graph::GraphNode* reconfiguredSource = RequireNode(graph, sourceId);
    return
        Check(reconfigured, "valid source reconfiguration should succeed") &&
        Check(graph.edges.empty(), "reconfiguration should remove stale incident edges") &&
        Check(reconfiguredSource != nullptr, "reconfigured source should still exist") &&
        Check(reconfiguredSource->recipeId == graph::production_ids::SupplyWater, "source recipe id should update") &&
        Check(FindOutputByProductionResource(*reconfiguredSource, graph::production_ids::Water) != nullptr, "source should now output water");
}

bool CheckGeneratedPortsPreserveConnectionValidation()
{
    // Verifies that generated ports participate in the existing connection rules:
    // matching production resources connect, mismatches reject, and byproduct
    // outputs can connect to compatible sink inputs.
    const CatalogSet catalogs;
    graph::GraphDocument graph;

    const int sourceId = graph::AddRecipeNode(
        graph,
        graph::production_ids::ResourceSource,
        graph::production_ids::ExtractIronOre,
        catalogs.machines,
        catalogs.recipes,
        catalogs.resources
    );

    const int crusherId = graph::AddRecipeNode(
        graph,
        graph::production_ids::Crusher,
        graph::production_ids::CrushIronOre,
        catalogs.machines,
        catalogs.recipes,
        catalogs.resources
    );

    const int washerId = graph::AddRecipeNode(
        graph,
        graph::production_ids::Washer,
        graph::production_ids::WashCrushedIronOre,
        catalogs.machines,
        catalogs.recipes,
        catalogs.resources
    );

    const int wasteSinkId = graph::AddRecipeNode(
        graph,
        graph::production_ids::WasteSink,
        graph::production_ids::StoreIronSlurry,
        catalogs.machines,
        catalogs.recipes,
        catalogs.resources
    );

    const graph::GraphNode* source = RequireNode(graph, sourceId);
    const graph::GraphNode* crusher = RequireNode(graph, crusherId);
    const graph::GraphNode* washer = RequireNode(graph, washerId);
    const graph::GraphNode* wasteSink = RequireNode(graph, wasteSinkId);
    if (!Check(source != nullptr, "source should exist") ||
        !Check(crusher != nullptr, "crusher should exist") ||
        !Check(washer != nullptr, "washer should exist") ||
        !Check(wasteSink != nullptr, "waste sink should exist"))
    {
        return false;
    }

    const graph::GraphPort* sourceIron = FindOutputByProductionResource(*source, graph::production_ids::IronOre);
    const graph::GraphPort* crusherIronInput = FindInputByProductionResource(*crusher, graph::production_ids::IronOre);
    const graph::GraphPort* crusherCrushedOutput = FindOutputByProductionResource(*crusher, graph::production_ids::CrushedIronOre);
    const graph::GraphPort* washerCrushedInput = FindInputByProductionResource(*washer, graph::production_ids::CrushedIronOre);
    const graph::GraphPort* washerWaterInput = FindInputByProductionResource(*washer, graph::production_ids::Water);
    const graph::GraphPort* washerSlurryOutput = FindOutputByProductionResource(*washer, graph::production_ids::IronSlurry);
    const graph::GraphPort* wasteSinkSlurryInput = FindInputByProductionResource(*wasteSink, graph::production_ids::IronSlurry);

    if (!Check(sourceIron != nullptr, "source iron output should exist") ||
        !Check(crusherIronInput != nullptr, "crusher iron input should exist") ||
        !Check(crusherCrushedOutput != nullptr, "crusher crushed output should exist") ||
        !Check(washerCrushedInput != nullptr, "washer crushed input should exist") ||
        !Check(washerWaterInput != nullptr, "washer water input should exist") ||
        !Check(washerSlurryOutput != nullptr, "washer slurry output should exist") ||
        !Check(wasteSinkSlurryInput != nullptr, "waste sink slurry input should exist"))
    {
        return false;
    }

    return
        Check(graph::CanConnect(graph, sourceId, sourceIron->id, crusherId, crusherIronInput->id), "iron source should connect to crusher input") &&
        Check(graph::CanConnect(graph, crusherId, crusherCrushedOutput->id, washerId, washerCrushedInput->id), "crusher output should connect to washer input") &&
        Check(!graph::CanConnect(graph, crusherId, crusherCrushedOutput->id, washerId, washerWaterInput->id), "crushed ore should not connect to water input") &&
        Check(graph::CanConnect(graph, washerId, washerSlurryOutput->id, wasteSinkId, wasteSinkSlurryInput->id), "slurry byproduct should connect to waste sink") &&
        Check(graph::AddEdge(graph, washerId, washerSlurryOutput->id, wasteSinkId, wasteSinkSlurryInput->id) != 0, "slurry sink edge should be created");
}

bool CheckSampleGraphUsesRecipeDrivenTier0Chain()
{
    // Protects the startup sample from drifting back to hand-authored legacy
    // ports. The sample should demonstrate the Tier 0 puzzle chain using catalog
    // recipe nodes while a temporary output node stands in for future objectives.
    const graph::GraphDocument graph = graph::CreateSampleFactoryGraph();

    const graph::GraphNode* ironSource = FindNodeByRecipe(
        graph,
        graph::production_ids::ResourceSource,
        graph::production_ids::ExtractIronOre
    );
    const graph::GraphNode* waterSource = FindNodeByRecipe(
        graph,
        graph::production_ids::ResourceSource,
        graph::production_ids::SupplyWater
    );
    const graph::GraphNode* crusher = FindNodeByRecipe(
        graph,
        graph::production_ids::Crusher,
        graph::production_ids::CrushIronOre
    );
    const graph::GraphNode* washer = FindNodeByRecipe(
        graph,
        graph::production_ids::Washer,
        graph::production_ids::WashCrushedIronOre
    );
    const graph::GraphNode* smelter = FindNodeByRecipe(
        graph,
        graph::production_ids::Smelter,
        graph::production_ids::SmeltWashedIronOre
    );
    const graph::GraphNode* wasteSink = FindNodeByRecipe(
        graph,
        graph::production_ids::WasteSink,
        graph::production_ids::StoreIronSlurry
    );
    const graph::GraphNode* ironTarget = FindOutputNodeForResource(graph, graph::production_ids::IronIngot);

    if (!Check(ironSource != nullptr, "sample should include iron source recipe node") ||
        !Check(waterSource != nullptr, "sample should include water source recipe node") ||
        !Check(crusher != nullptr, "sample should include crusher recipe node") ||
        !Check(washer != nullptr, "sample should include washer recipe node") ||
        !Check(smelter != nullptr, "sample should include smelter recipe node") ||
        !Check(wasteSink != nullptr, "sample should include waste sink recipe node") ||
        !Check(ironTarget != nullptr, "sample should include temporary iron target node"))
    {
        return false;
    }

    const graph::GraphPort* slurryOutput = FindOutputByProductionResource(*washer, graph::production_ids::IronSlurry);
    if (!Check(slurryOutput != nullptr, "sample washer should expose slurry byproduct output") ||
        !Check(slurryOutput->isByproduct, "sample washer slurry output should be tagged as byproduct"))
    {
        return false;
    }

    return
        Check(graph.nodes.size() == 7, "sample should contain six recipe nodes plus one temporary target") &&
        Check(graph.edges.size() == 6, "sample should contain six Tier 0 production links") &&
        Check(ironTarget->machineId == graph::InvalidMachineId, "temporary target should not pretend to be catalog machine") &&
        Check(ironTarget->recipeId == graph::InvalidRecipeId, "temporary target should not pretend to be catalog recipe") &&
        Check(HasEdge(graph, ironSource->id, graph::production_ids::IronOre, crusher->id, graph::production_ids::IronOre), "sample should connect iron source to crusher") &&
        Check(HasEdge(graph, crusher->id, graph::production_ids::CrushedIronOre, washer->id, graph::production_ids::CrushedIronOre), "sample should connect crusher to washer") &&
        Check(HasEdge(graph, waterSource->id, graph::production_ids::Water, washer->id, graph::production_ids::Water), "sample should connect water source to washer") &&
        Check(HasEdge(graph, washer->id, graph::production_ids::WashedIronOre, smelter->id, graph::production_ids::WashedIronOre), "sample should connect washer to smelter") &&
        Check(HasEdge(graph, smelter->id, graph::production_ids::IronIngot, ironTarget->id, graph::production_ids::IronIngot), "sample should connect smelter to temporary target") &&
        Check(HasEdge(graph, washer->id, graph::production_ids::IronSlurry, wasteSink->id, graph::production_ids::IronSlurry), "sample should route slurry byproduct to waste sink");
}

} // namespace

int RunGraphRecipeNodeTest()
{
    if (!CheckCrusherRecipeNode() ||
        !CheckWasherRecipeNodeWithByproduct() ||
        !CheckInvalidMachineRecipePairIsRejected() ||
        !CheckReconfigureRemovesStaleEdges() ||
        !CheckGeneratedPortsPreserveConnectionValidation() ||
        !CheckSampleGraphUsesRecipeDrivenTier0Chain())
    {
        return 1;
    }

    std::cout << "Graph recipe-node test passed.\n";
    return 0;
}
