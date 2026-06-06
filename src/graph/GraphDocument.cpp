// Implements graph document lookup, mutation, connection validation, and the
// bridge from production catalog recipes to generated graph ports. This file does
// not perform rendering or solve production flow; it only maintains graph data.

#include "graph/GraphDocument.h"

#include <algorithm>
#include <utility>

namespace graph
{
namespace
{
GraphPort MakePort(
    int id,
    std::string name,
    ResourceType resource,
    PortDirection direction,
    ResourceId productionResourceId,
    bool isByproduct
)
{
    return GraphPort{
        .id = id,
        .name = std::move(name),
        .resource = resource,
        .direction = direction,
        .productionResourceId = productionResourceId,
        .isByproduct = isByproduct
    };
}

ResourceType LegacyResourceTypeForProductionResource(ResourceId resourceId)
{
    // Keeps recipe-generated ports compatible with the current renderer and
    // legacy evaluator while the production catalog becomes the long-term source
    // of truth for resources.
    switch (resourceId)
    {
        case production_ids::IronOre: return ResourceType::IronOre;
        case production_ids::CrushedIronOre: return ResourceType::CrushedOre;
        case production_ids::WashedIronOre: return ResourceType::WashedOre;
        case production_ids::IronSlurry: return ResourceType::Slurry;
        case production_ids::IronIngot: return ResourceType::IronIngot;
        case production_ids::Water: return ResourceType::Water;
        case production_ids::Waste: return ResourceType::Waste;
        default: return ResourceType::IronOre;
    }
}

NodeType NodeTypeForMachineClass(MachineClass machineClass)
{
    switch (machineClass)
    {
        case MachineClass::Source: return NodeType::Source;
        case MachineClass::Storage:
        case MachineClass::WasteHandling:
            return NodeType::Storage;
        case MachineClass::Crushing:
        case MachineClass::Washing:
        case MachineClass::Smelting:
        case MachineClass::Mixing:
        case MachineClass::Separating:
        case MachineClass::Chemical:
        case MachineClass::Refining:
            return NodeType::Machine;
    }

    return NodeType::Machine;
}

bool ResourceExists(const ResourceCatalog& resources, ResourceId resourceId)
{
    return resources.Find(resourceId) != nullptr;
}

bool RecipeResourcesExist(const RecipeDef& recipe, const ResourceCatalog& resources)
{
    // Validate all recipe resource references before mutating the graph so a bad
    // catalog entry cannot leave a half-generated node behind.
    for (const ResourceAmount& amount : recipe.inputs)
    {
        if (!ResourceExists(resources, amount.resourceId))
        {
            return false;
        }
    }

    for (const ResourceAmount& amount : recipe.outputs)
    {
        if (!ResourceExists(resources, amount.resourceId))
        {
            return false;
        }
    }

    for (const ResourceAmount& amount : recipe.byproducts)
    {
        if (!ResourceExists(resources, amount.resourceId))
        {
            return false;
        }
    }

    return true;
}

bool ResolveRecipeNodeDefinition(
    MachineId machineId,
    RecipeId recipeId,
    const MachineCatalog& machines,
    const RecipeCatalog& recipes,
    const ResourceCatalog& resources,
    const MachineDef*& machine,
    const RecipeDef*& recipe
)
{
    machine = machines.Find(machineId);
    recipe = recipes.Find(recipeId);

    if (machine == nullptr || recipe == nullptr)
    {
        return false;
    }

    if (machine->machineClass != recipe->requiredMachineClass)
    {
        return false;
    }

    return RecipeResourcesExist(*recipe, resources);
}

void RemoveEdgesTouchingNode(GraphDocument& graph, int nodeId)
{
    // Regenerated ports receive new IDs, so all existing links attached to the
    // node must be removed to avoid dangling edge endpoints.
    graph.edges.erase(
        std::remove_if(graph.edges.begin(), graph.edges.end(), [nodeId](const GraphEdge& edge) {
            return edge.fromNodeId == nodeId || edge.toNodeId == nodeId;
        }),
        graph.edges.end()
    );
}

void AddPortsForAmounts(
    GraphDocument& graph,
    int nodeId,
    const ResourceCatalog& resources,
    const std::vector<ResourceAmount>& amounts,
    PortDirection direction,
    bool isByproduct
)
{
    for (const ResourceAmount& amount : amounts)
    {
        const ResourceDef* resource = resources.Find(amount.resourceId);
        if (resource == nullptr)
        {
            continue;
        }

        const ResourceType legacyResource = LegacyResourceTypeForProductionResource(amount.resourceId);

        if (direction == PortDirection::Input)
        {
            (void)AddInputPort(graph, nodeId, resource->displayName, legacyResource, amount.resourceId);
        }
        else
        {
            (void)AddOutputPort(
                graph,
                nodeId,
                resource->displayName,
                legacyResource,
                amount.resourceId,
                isByproduct
            );
        }
    }
}

bool PortsUseProductionResourceIds(const GraphPort& fromPort, const GraphPort& toPort)
{
    return
        fromPort.productionResourceId != InvalidResourceId &&
        toPort.productionResourceId != InvalidResourceId;
}

bool PortsHaveCompatibleResources(const GraphPort& fromPort, const GraphPort& toPort)
{
    if (PortsUseProductionResourceIds(fromPort, toPort))
    {
        return fromPort.productionResourceId == toPort.productionResourceId;
    }

    return fromPort.resource == toPort.resource;
}
} // namespace

GraphNode* FindNode(GraphDocument& graph, int nodeId)
{
    auto it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [nodeId](const GraphNode& node) {
        return node.id == nodeId;
    });

    return it == graph.nodes.end() ? nullptr : &(*it);
}

const GraphNode* FindNode(const GraphDocument& graph, int nodeId)
{
    auto it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [nodeId](const GraphNode& node) {
        return node.id == nodeId;
    });

    return it == graph.nodes.end() ? nullptr : &(*it);
}

GraphPort* FindPort(GraphNode& node, int portId)
{
    auto inputIt = std::find_if(node.inputs.begin(), node.inputs.end(), [portId](const GraphPort& port) {
        return port.id == portId;
    });

    if (inputIt != node.inputs.end())
    {
        return &(*inputIt);
    }

    auto outputIt = std::find_if(node.outputs.begin(), node.outputs.end(), [portId](const GraphPort& port) {
        return port.id == portId;
    });

    return outputIt == node.outputs.end() ? nullptr : &(*outputIt);
}

const GraphPort* FindPort(const GraphNode& node, int portId)
{
    auto inputIt = std::find_if(node.inputs.begin(), node.inputs.end(), [portId](const GraphPort& port) {
        return port.id == portId;
    });

    if (inputIt != node.inputs.end())
    {
        return &(*inputIt);
    }

    auto outputIt = std::find_if(node.outputs.begin(), node.outputs.end(), [portId](const GraphPort& port) {
        return port.id == portId;
    });

    return outputIt == node.outputs.end() ? nullptr : &(*outputIt);
}

const GraphPort* FindPort(const GraphDocument& graph, int nodeId, int portId)
{
    const GraphNode* node = FindNode(graph, nodeId);
    return node == nullptr ? nullptr : FindPort(*node, portId);
}

int AddNode(GraphDocument& graph, NodeType type, std::string name)
{
    const int id = graph.nextNodeId++;

    graph.nodes.push_back(GraphNode{
        .id = id,
        .type = type,
        .name = std::move(name)
    });

    return id;
}

int AddInputPort(
    GraphDocument& graph,
    int nodeId,
    std::string name,
    ResourceType resource,
    ResourceId productionResourceId
)
{
    GraphNode* node = FindNode(graph, nodeId);

    if (node == nullptr)
    {
        return 0;
    }

    const int id = graph.nextPortId++;
    node->inputs.push_back(MakePort(
        id,
        std::move(name),
        resource,
        PortDirection::Input,
        productionResourceId,
        false
    ));
    return id;
}

int AddOutputPort(
    GraphDocument& graph,
    int nodeId,
    std::string name,
    ResourceType resource,
    ResourceId productionResourceId,
    bool isByproduct
)
{
    GraphNode* node = FindNode(graph, nodeId);

    if (node == nullptr)
    {
        return 0;
    }

    const int id = graph.nextPortId++;
    node->outputs.push_back(MakePort(
        id,
        std::move(name),
        resource,
        PortDirection::Output,
        productionResourceId,
        isByproduct
    ));
    return id;
}

int AddRecipeNode(
    GraphDocument& graph,
    MachineId machineId,
    RecipeId recipeId,
    const MachineCatalog& machines,
    const RecipeCatalog& recipes,
    const ResourceCatalog& resources
)
{
    const MachineDef* machine = nullptr;
    const RecipeDef* recipe = nullptr;
    if (!ResolveRecipeNodeDefinition(machineId, recipeId, machines, recipes, resources, machine, recipe))
    {
        return 0;
    }

    const int nodeId = graph.nextNodeId++;
    graph.nodes.push_back(GraphNode{
        .id = nodeId,
        .type = NodeTypeForMachineClass(machine->machineClass),
        .name = machine->displayName,
        .machineId = machine->id,
        .recipeId = recipe->id,
        .capacity = machine->baseThroughputPerMinute,
        .powerUse = recipe->powerKw
    });

    AddPortsForAmounts(graph, nodeId, resources, recipe->inputs, PortDirection::Input, false);
    AddPortsForAmounts(graph, nodeId, resources, recipe->outputs, PortDirection::Output, false);
    AddPortsForAmounts(graph, nodeId, resources, recipe->byproducts, PortDirection::Output, true);

    return nodeId;
}

bool ConfigureNodeFromRecipe(
    GraphDocument& graph,
    int nodeId,
    MachineId machineId,
    RecipeId recipeId,
    const MachineCatalog& machines,
    const RecipeCatalog& recipes,
    const ResourceCatalog& resources
)
{
    GraphNode* node = FindNode(graph, nodeId);
    if (node == nullptr)
    {
        return false;
    }

    const MachineDef* machine = nullptr;
    const RecipeDef* recipe = nullptr;
    if (!ResolveRecipeNodeDefinition(machineId, recipeId, machines, recipes, resources, machine, recipe))
    {
        return false;
    }

    RemoveEdgesTouchingNode(graph, nodeId);

    node->type = NodeTypeForMachineClass(machine->machineClass);
    node->name = machine->displayName;
    node->machineId = machine->id;
    node->recipeId = recipe->id;
    node->capacity = machine->baseThroughputPerMinute;
    node->powerUse = recipe->powerKw;
    node->inputs.clear();
    node->outputs.clear();

    AddPortsForAmounts(graph, nodeId, resources, recipe->inputs, PortDirection::Input, false);
    AddPortsForAmounts(graph, nodeId, resources, recipe->outputs, PortDirection::Output, false);
    AddPortsForAmounts(graph, nodeId, resources, recipe->byproducts, PortDirection::Output, true);

    return true;
}

bool CanConnect(
    const GraphDocument& graph,
    int fromNodeId,
    int fromPortId,
    int toNodeId,
    int toPortId
)
{
    if (fromNodeId == toNodeId)
    {
        return false;
    }

    const GraphPort* fromPort = FindPort(graph, fromNodeId, fromPortId);
    const GraphPort* toPort = FindPort(graph, toNodeId, toPortId);

    if (fromPort == nullptr || toPort == nullptr)
    {
        return false;
    }

    if (fromPort->direction != PortDirection::Output || toPort->direction != PortDirection::Input)
    {
        return false;
    }

    if (!PortsHaveCompatibleResources(*fromPort, *toPort))
    {
        return false;
    }

    const auto duplicate = std::find_if(graph.edges.begin(), graph.edges.end(), [&](const GraphEdge& edge) {
        return edge.fromNodeId == fromNodeId &&
               edge.fromPortId == fromPortId &&
               edge.toNodeId == toNodeId &&
               edge.toPortId == toPortId;
    });

    return duplicate == graph.edges.end();
}

int AddEdge(
    GraphDocument& graph,
    int fromNodeId,
    int fromPortId,
    int toNodeId,
    int toPortId
)
{
    if (!CanConnect(graph, fromNodeId, fromPortId, toNodeId, toPortId))
    {
        return 0;
    }

    const int id = graph.nextEdgeId++;
    graph.edges.push_back(GraphEdge{
        .id = id,
        .fromNodeId = fromNodeId,
        .fromPortId = fromPortId,
        .toNodeId = toNodeId,
        .toPortId = toPortId
    });

    return id;
}

void RemoveEdge(GraphDocument& graph, int edgeId)
{
    graph.edges.erase(
        std::remove_if(graph.edges.begin(), graph.edges.end(), [edgeId](const GraphEdge& edge) {
            return edge.id == edgeId;
        }),
        graph.edges.end()
    );
}

void RemoveNode(GraphDocument& graph, int nodeId)
{
    graph.nodes.erase(
        std::remove_if(graph.nodes.begin(), graph.nodes.end(), [nodeId](const GraphNode& node) {
            return node.id == nodeId;
        }),
        graph.nodes.end()
    );

    RemoveEdgesTouchingNode(graph, nodeId);
}

} // namespace graph
