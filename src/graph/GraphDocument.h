#pragma once

// Declares graph editing helpers used by the editor, tests, sample graph, and
// upcoming production-catalog integration. This layer owns graph mutation only;
// rendering, serialization, and evaluation are handled by separate systems.

#include "graph/GraphTypes.h"
#include "graph/MachineCatalog.h"
#include "graph/RecipeCatalog.h"
#include "graph/ResourceCatalog.h"

#include <string>

namespace graph
{

// Finds a mutable node by document ID. Returns nullptr when the node does not
// exist; the returned pointer is non-owning and valid until the node list mutates.
[[nodiscard]] GraphNode* FindNode(GraphDocument& graph, int nodeId);

// Finds a read-only node by document ID. Returns nullptr when the node does not
// exist; the returned pointer is non-owning and valid until the node list mutates.
[[nodiscard]] const GraphNode* FindNode(const GraphDocument& graph, int nodeId);

// Finds a mutable input or output port on one node. Returns nullptr when the
// port ID is unknown; the returned pointer is non-owning and invalidated by port
// vector mutation.
[[nodiscard]] GraphPort* FindPort(GraphNode& node, int portId);

// Finds a read-only input or output port on one node. Returns nullptr when the
// port ID is unknown.
[[nodiscard]] const GraphPort* FindPort(const GraphNode& node, int portId);

// Finds a read-only port by node ID and port ID. Returns nullptr when either ID
// is unknown.
[[nodiscard]] const GraphPort* FindPort(const GraphDocument& graph, int nodeId, int portId);

// Adds a legacy/editor node and returns its stable node ID. Recipe-driven code
// should prefer AddRecipeNode when machine and recipe IDs are known up front.
[[nodiscard]] int AddNode(GraphDocument& graph, NodeType type, std::string name);

// Adds an input port to an existing node. Returns zero when the node ID is
// unknown. productionResourceId is optional for legacy hand-authored sample ports.
[[nodiscard]] int AddInputPort(
    GraphDocument& graph,
    int nodeId,
    std::string name,
    ResourceType resource,
    ResourceId productionResourceId = InvalidResourceId
);

// Adds an output port to an existing node. Returns zero when the node ID is
// unknown. Byproduct ports are still ordinary output ports, but the flag lets the
// future evaluator apply byproduct-handling objective rules.
[[nodiscard]] int AddOutputPort(
    GraphDocument& graph,
    int nodeId,
    std::string name,
    ResourceType resource,
    ResourceId productionResourceId = InvalidResourceId,
    bool isByproduct = false
);

// Creates a machine node configured with a catalog recipe. The node is only
// appended after machine/recipe compatibility and all referenced resources are
// validated, so failures do not consume graph IDs.
[[nodiscard]] int AddRecipeNode(
    GraphDocument& graph,
    MachineId machineId,
    RecipeId recipeId,
    const MachineCatalog& machines,
    const RecipeCatalog& recipes,
    const ResourceCatalog& resources
);

// Replaces an existing node's machine/recipe identity and regenerates its ports.
// Existing edges attached to the node are removed because port IDs are rebuilt
// from the selected recipe. Returns false without mutating the node on invalid
// machine/recipe/resource data.
[[nodiscard]] bool ConfigureNodeFromRecipe(
    GraphDocument& graph,
    int nodeId,
    MachineId machineId,
    RecipeId recipeId,
    const MachineCatalog& machines,
    const RecipeCatalog& recipes,
    const ResourceCatalog& resources
);

// Validates connection direction, resource compatibility, self-connections, and
// duplicate edges. Production ResourceId values are preferred when both ports
// have them; otherwise legacy ResourceType compatibility is used.
[[nodiscard]] bool CanConnect(
    const GraphDocument& graph,
    int fromNodeId,
    int fromPortId,
    int toNodeId,
    int toPortId
);

// Adds a validated edge and returns its ID. Returns zero and leaves the graph
// unchanged when CanConnect rejects the requested connection.
[[nodiscard]] int AddEdge(
    GraphDocument& graph,
    int fromNodeId,
    int fromPortId,
    int toNodeId,
    int toPortId
);

// Removes the edge with the requested ID. Missing IDs are ignored so callers can
// safely issue cleanup after selection state has already changed.
void RemoveEdge(GraphDocument& graph, int edgeId);

// Removes one node and all incident edges. Missing node IDs are ignored.
void RemoveNode(GraphDocument& graph, int nodeId);

} // namespace graph
