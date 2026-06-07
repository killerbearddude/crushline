// Implements the MVP production evaluator described in docs/game-design.md. The
// evaluator is intentionally small and deterministic: it rejects cycles, enforces
// one edge per port, propagates rates in topological order, and evaluates typed
// scenario objectives.

#include "graph/ProductionEvaluator.h"

#include "graph/GraphDocument.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <sstream>
#include <utility>

namespace graph
{
namespace
{
constexpr float RateEpsilon = 0.0001f;

struct ValidatedEdge
{
    const GraphEdge* edge = nullptr;
    bool usableForFlow = false;
};

struct RuntimeNode
{
    const GraphNode* node = nullptr;
    const MachineDef* machine = nullptr;
    const RecipeDef* recipe = nullptr;
    float utilization = 0.0f;
    std::map<ResourceId, float> outputRates;
};

float Clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

bool NearlyZero(float value)
{
    return std::fabs(value) <= RateEpsilon;
}

std::string FormatRate(float rate)
{
    std::ostringstream stream;
    stream << rate << "/min";
    return stream.str();
}

std::string ResourceName(const ResourceCatalog& resources, ResourceId resourceId)
{
    const ResourceDef* resource = resources.Find(resourceId);
    return resource == nullptr ? "Unknown Resource" : resource->displayName;
}

const GraphPort* FindInputPortByResource(const GraphNode& node, ResourceId resourceId)
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


const GraphEdge* FindFirstIncomingEdge(
    const std::vector<ValidatedEdge>& edges,
    int nodeId,
    int portId
)
{
    for (const ValidatedEdge& validated : edges)
    {
        if (!validated.usableForFlow || validated.edge == nullptr)
        {
            continue;
        }

        if (validated.edge->toNodeId == nodeId && validated.edge->toPortId == portId)
        {
            return validated.edge;
        }
    }

    return nullptr;
}

void AddResourceProduced(
    std::map<ResourceId, ResourceFlowSummary>& summaries,
    ResourceId resourceId,
    float rate
)
{
    ResourceFlowSummary& summary = summaries[resourceId];
    summary.resourceId = resourceId;
    summary.producedPerMinute += rate;
}

void AddResourceConsumed(
    std::map<ResourceId, ResourceFlowSummary>& summaries,
    ResourceId resourceId,
    float rate
)
{
    ResourceFlowSummary& summary = summaries[resourceId];
    summary.resourceId = resourceId;
    summary.consumedPerMinute += rate;
}

void AddResourceDeficit(
    std::map<ResourceId, ResourceFlowSummary>& summaries,
    ResourceId resourceId,
    float rate
)
{
    if (rate <= RateEpsilon)
    {
        return;
    }

    ResourceFlowSummary& summary = summaries[resourceId];
    summary.resourceId = resourceId;
    summary.deficitPerMinute += rate;
}

bool PortsHaveExactProductionMatch(const GraphPort& fromPort, const GraphPort& toPort)
{
    return fromPort.productionResourceId != InvalidResourceId &&
           toPort.productionResourceId != InvalidResourceId &&
           fromPort.productionResourceId == toPort.productionResourceId;
}

bool ValidateEndpointAndResource(
    const GraphDocument& graph,
    const GraphEdge& edge,
    ProductionEvaluation& result
)
{
    if (edge.fromNodeId == edge.toNodeId)
    {
        ++result.invalidConnectionCount;
        result.events.push_back("Invalid production link: same node");
        return false;
    }

    const GraphPort* fromPort = FindPort(graph, edge.fromNodeId, edge.fromPortId);
    const GraphPort* toPort = FindPort(graph, edge.toNodeId, edge.toPortId);

    if (fromPort == nullptr || toPort == nullptr)
    {
        ++result.invalidConnectionCount;
        result.events.push_back("Invalid production link: missing endpoint");
        return false;
    }

    if (fromPort->direction != PortDirection::Output || toPort->direction != PortDirection::Input)
    {
        ++result.invalidConnectionCount;
        result.events.push_back("Invalid production link: output must target input");
        return false;
    }

    if (!PortsHaveExactProductionMatch(*fromPort, *toPort))
    {
        ++result.invalidConnectionCount;
        result.events.push_back("Invalid production link: resource mismatch");
        return false;
    }

    return true;
}

std::vector<ValidatedEdge> ValidateEdges(const GraphDocument& graph, ProductionEvaluation& result)
{
    std::vector<ValidatedEdge> validatedEdges;
    validatedEdges.reserve(graph.edges.size());

    std::map<int, int> outputUseCount;
    std::map<int, int> inputUseCount;

    for (const GraphEdge& edge : graph.edges)
    {
        const bool endpointValid = ValidateEndpointAndResource(graph, edge, result);
        validatedEdges.push_back(ValidatedEdge{&edge, endpointValid});

        if (endpointValid)
        {
            ++outputUseCount[edge.fromPortId];
            ++inputUseCount[edge.toPortId];
        }
    }

    for (ValidatedEdge& validated : validatedEdges)
    {
        if (!validated.usableForFlow || validated.edge == nullptr)
        {
            continue;
        }

        // MVP graphs do not have implicit splitters or mergers. The evaluator
        // marks excess links invalid and ignores them for flow so results remain
        // deterministic even when the editor currently allows those links.
        const GraphEdge& edge = *validated.edge;
        if (outputUseCount[edge.fromPortId] > 1 || inputUseCount[edge.toPortId] > 1)
        {
            ++result.invalidConnectionCount;
            result.events.push_back("Invalid production link: one edge per port in MVP");
            validated.usableForFlow = false;
        }
    }

    return validatedEdges;
}

std::vector<int> BuildTopologicalOrder(
    const GraphDocument& graph,
    const std::vector<ValidatedEdge>& edges,
    ProductionEvaluation& result
)
{
    std::map<int, int> indegree;
    std::map<int, std::vector<int>> outgoing;

    for (const GraphNode& node : graph.nodes)
    {
        indegree[node.id] = 0;
    }

    for (const ValidatedEdge& validated : edges)
    {
        if (!validated.usableForFlow || validated.edge == nullptr)
        {
            continue;
        }

        outgoing[validated.edge->fromNodeId].push_back(validated.edge->toNodeId);
        ++indegree[validated.edge->toNodeId];
    }

    std::queue<int> ready;
    for (const auto& [nodeId, degree] : indegree)
    {
        if (degree == 0)
        {
            ready.push(nodeId);
        }
    }

    std::vector<int> order;
    order.reserve(graph.nodes.size());

    while (!ready.empty())
    {
        const int nodeId = ready.front();
        ready.pop();
        order.push_back(nodeId);

        for (const int nextNodeId : outgoing[nodeId])
        {
            --indegree[nextNodeId];
            if (indegree[nextNodeId] == 0)
            {
                ready.push(nextNodeId);
            }
        }
    }

    result.hasCycle = order.size() != graph.nodes.size();
    if (result.hasCycle)
    {
        result.events.push_back("Production graph contains a cycle; cycles are invalid for MVP");
    }

    return order;
}

std::map<int, RuntimeNode> BuildRuntimeNodes(
    const GraphDocument& graph,
    const MachineCatalog& machines,
    const RecipeCatalog& recipes,
    ProductionEvaluation& result
)
{
    std::map<int, RuntimeNode> runtimeNodes;

    for (const GraphNode& node : graph.nodes)
    {
        RuntimeNode runtime;
        runtime.node = &node;

        if (node.machineId != InvalidMachineId || node.recipeId != InvalidRecipeId)
        {
            runtime.machine = machines.Find(node.machineId);
            runtime.recipe = recipes.Find(node.recipeId);

            if (runtime.machine == nullptr || runtime.recipe == nullptr)
            {
                ++result.invalidConnectionCount;
                result.events.push_back("Invalid production node: missing machine or recipe definition");
            }
            else if (runtime.machine->machineClass != runtime.recipe->requiredMachineClass)
            {
                ++result.invalidConnectionCount;
                result.events.push_back("Invalid production node: machine cannot run recipe");
                runtime.recipe = nullptr;
            }
        }

        runtimeNodes[node.id] = runtime;
    }

    return runtimeNodes;
}

float OutputRateForResource(const RuntimeNode& runtime, ResourceId resourceId)
{
    const auto it = runtime.outputRates.find(resourceId);
    return it == runtime.outputRates.end() ? 0.0f : it->second;
}

float IncomingAvailableRate(
    const GraphDocument& graph,
    const std::vector<ValidatedEdge>& edges,
    const std::map<int, RuntimeNode>& runtimeNodes,
    const GraphNode& node,
    const GraphPort& inputPort
)
{
    const GraphEdge* edge = FindFirstIncomingEdge(edges, node.id, inputPort.id);
    if (edge == nullptr)
    {
        return 0.0f;
    }

    const GraphPort* fromPort = FindPort(graph, edge->fromNodeId, edge->fromPortId);
    if (fromPort == nullptr)
    {
        return 0.0f;
    }

    const auto fromRuntimeIt = runtimeNodes.find(edge->fromNodeId);
    if (fromRuntimeIt == runtimeNodes.end())
    {
        return 0.0f;
    }

    return OutputRateForResource(fromRuntimeIt->second, fromPort->productionResourceId);
}

void EvaluateRuntimeNode(
    const GraphDocument& graph,
    const ResourceCatalog& resources,
    const std::vector<ValidatedEdge>& edges,
    std::map<int, RuntimeNode>& runtimeNodes,
    RuntimeNode& runtime,
    ProductionEvaluation& result,
    std::map<ResourceId, ResourceFlowSummary>& resourceSummaries
)
{
    if (runtime.node == nullptr || runtime.recipe == nullptr)
    {
        return;
    }

    struct InputAvailability
    {
        ResourceId resourceId = InvalidResourceId;
        float requiredRate = 0.0f;
        float availableRate = 0.0f;
    };

    NodeProductionEval nodeEval;
    nodeEval.nodeId = runtime.node->id;

    float utilization = 1.0f;
    std::vector<InputAvailability> inputs;
    inputs.reserve(runtime.recipe->inputs.size());

    for (const ResourceAmount& input : runtime.recipe->inputs)
    {
        const GraphPort* inputPort = FindInputPortByResource(*runtime.node, input.resourceId);
        const float availableRate = inputPort == nullptr
            ? 0.0f
            : IncomingAvailableRate(graph, edges, runtimeNodes, *runtime.node, *inputPort);

        const float requiredRate = input.ratePerMinute;
        const float inputRatio = requiredRate > RateEpsilon ? availableRate / requiredRate : 1.0f;
        utilization = std::min(utilization, Clamp01(inputRatio));

        inputs.push_back(InputAvailability{input.resourceId, requiredRate, availableRate});

        if (availableRate + RateEpsilon < requiredRate)
        {
            nodeEval.messages.push_back(
                "Missing " + ResourceName(resources, input.resourceId) + " input"
            );
        }
    }

    // Machines consume all required inputs at the same utilization. This avoids
    // incorrectly consuming one available input when another required input is
    // missing and the recipe cannot actually run.
    for (const InputAvailability& input : inputs)
    {
        AddResourceConsumed(resourceSummaries, input.resourceId, input.requiredRate * utilization);
        AddResourceDeficit(resourceSummaries, input.resourceId, std::max(0.0f, input.requiredRate - input.availableRate));
    }

    runtime.utilization = utilization;
    nodeEval.utilization = utilization;
    nodeEval.active = utilization > RateEpsilon;
    nodeEval.starved = !runtime.recipe->inputs.empty() && NearlyZero(utilization);
    nodeEval.bottleneck = utilization > RateEpsilon && utilization + RateEpsilon < 1.0f;

    if (nodeEval.starved)
    {
        nodeEval.messages.push_back("Node is starved");
    }

    if (nodeEval.bottleneck)
    {
        nodeEval.messages.push_back("Node is input-limited");
        ++result.bottleneckCount;
    }

    const float throughputMultiplier = 1.0f;
    for (const ResourceAmount& output : runtime.recipe->outputs)
    {
        const float rate = output.ratePerMinute * utilization * throughputMultiplier;
        runtime.outputRates[output.resourceId] += rate;
        AddResourceProduced(resourceSummaries, output.resourceId, rate);
    }

    for (const ResourceAmount& byproduct : runtime.recipe->byproducts)
    {
        const float rate = byproduct.ratePerMinute * utilization * throughputMultiplier;
        runtime.outputRates[byproduct.resourceId] += rate;
        AddResourceProduced(resourceSummaries, byproduct.resourceId, rate);
    }

    result.totalPowerKw += runtime.recipe->powerKw * utilization;
    result.nodes.push_back(std::move(nodeEval));
}

ObjectiveStatus EvaluateProduceAtLeastObjective(
    const ResourceCatalog& resources,
    const ScenarioObjective& objective,
    const std::map<ResourceId, ResourceFlowSummary>& resourceSummaries
)
{
    ObjectiveStatus status;
    status.kind = objective.kind;
    status.resourceId = objective.resourceId;
    status.requiredPerMinute = objective.thresholdRatePerMinute;

    const auto it = resourceSummaries.find(objective.resourceId);
    status.actualPerMinute = it == resourceSummaries.end() ? 0.0f : it->second.producedPerMinute;
    status.satisfactionRatio = status.requiredPerMinute > RateEpsilon
        ? Clamp01(status.actualPerMinute / status.requiredPerMinute)
        : 1.0f;
    status.satisfied = status.actualPerMinute + RateEpsilon >= status.requiredPerMinute;
    status.message = ResourceName(resources, objective.resourceId) + " production " +
        FormatRate(status.actualPerMinute) + " / " + FormatRate(status.requiredPerMinute);

    return status;
}

ObjectiveStatus EvaluateHandleAllProducedObjective(
    const ResourceCatalog& resources,
    const ScenarioObjective& objective,
    const std::map<ResourceId, ResourceFlowSummary>& resourceSummaries
)
{
    ObjectiveStatus status;
    status.kind = objective.kind;
    status.resourceId = objective.resourceId;

    const auto it = resourceSummaries.find(objective.resourceId);
    if (it != resourceSummaries.end())
    {
        status.requiredPerMinute = it->second.producedPerMinute;
        status.actualPerMinute = std::min(it->second.consumedPerMinute, it->second.producedPerMinute);
    }

    status.satisfactionRatio = status.requiredPerMinute > RateEpsilon
        ? Clamp01(status.actualPerMinute / status.requiredPerMinute)
        : 1.0f;
    status.satisfied = status.actualPerMinute + RateEpsilon >= status.requiredPerMinute;
    status.message = ResourceName(resources, objective.resourceId) + " handled " +
        FormatRate(status.actualPerMinute) + " / " + FormatRate(status.requiredPerMinute);

    return status;
}

ObjectiveStatus EvaluateObjective(
    const ResourceCatalog& resources,
    const ScenarioObjective& objective,
    const std::map<ResourceId, ResourceFlowSummary>& resourceSummaries
)
{
    switch (objective.kind)
    {
        case ObjectiveKind::ProduceAtLeastRate:
            return EvaluateProduceAtLeastObjective(resources, objective, resourceSummaries);
        case ObjectiveKind::HandleAllProduced:
            return EvaluateHandleAllProducedObjective(resources, objective, resourceSummaries);
        case ObjectiveKind::MaxWasteRate:
        case ObjectiveKind::MaxMachineCount:
        case ObjectiveKind::MaxPowerKw:
            break;
    }

    ObjectiveStatus unsupported;
    unsupported.kind = objective.kind;
    unsupported.resourceId = objective.resourceId;
    unsupported.message = "Objective type is not implemented by the MVP evaluator";
    return unsupported;
}

void FinalizeResourceSummaries(
    const ResourceCatalog& resources,
    std::map<ResourceId, ResourceFlowSummary>& resourceSummaries,
    ProductionEvaluation& result
)
{
    for (auto& [resourceId, summary] : resourceSummaries)
    {
        summary.surplusPerMinute = std::max(0.0f, summary.producedPerMinute - summary.consumedPerMinute);

        const ResourceDef* resource = resources.Find(resourceId);
        if (resource != nullptr && resource->isWaste)
        {
            // Count unmanaged waste after consumption, not every intermediate
            // waste/byproduct transfer. This keeps handled Iron Slurry from
            // being double-counted once the Waste Sink converts it to Waste.
            result.totalWastePerMinute += summary.surplusPerMinute;
        }

        result.resources.push_back(summary);
    }
}

void EvaluateScenarioObjectives(
    const ResourceCatalog& resources,
    const ScenarioDef& scenario,
    std::map<ResourceId, ResourceFlowSummary>& resourceSummaries,
    ProductionEvaluation& result
)
{
    float satisfactionSum = 0.0f;
    bool allSatisfied = true;

    for (const ScenarioObjective& objective : scenario.objectives)
    {
        ObjectiveStatus status = EvaluateObjective(resources, objective, resourceSummaries);

        if (objective.kind == ObjectiveKind::ProduceAtLeastRate)
        {
            const float missingRate = std::max(0.0f, status.requiredPerMinute - status.actualPerMinute);
            AddResourceDeficit(resourceSummaries, objective.resourceId, missingRate);
        }

        satisfactionSum += status.satisfactionRatio;
        allSatisfied = allSatisfied && status.satisfied;
        result.events.push_back(status.message);
        result.objectives.push_back(std::move(status));
    }

    result.targetSatisfactionRatio = scenario.objectives.empty()
        ? 1.0f
        : satisfactionSum / static_cast<float>(scenario.objectives.size());

    result.scenarioComplete = allSatisfied &&
        result.invalidConnectionCount == 0 &&
        !result.hasCycle;
}
} // namespace

ProductionEvaluator::ProductionEvaluator(
    const ResourceCatalog& resources,
    const MachineCatalog& machines,
    const RecipeCatalog& recipes
)
    : m_resources(&resources)
    , m_machines(&machines)
    , m_recipes(&recipes)
{
}

ProductionEvaluation ProductionEvaluator::Evaluate(
    const GraphDocument& graph,
    const ScenarioDef& scenario
) const
{
    ProductionEvaluation result;

    const std::vector<ValidatedEdge> edges = ValidateEdges(graph, result);
    const std::vector<int> topoOrder = BuildTopologicalOrder(graph, edges, result);
    std::map<int, RuntimeNode> runtimeNodes = BuildRuntimeNodes(graph, *m_machines, *m_recipes, result);
    std::map<ResourceId, ResourceFlowSummary> resourceSummaries;

    for (const int nodeId : topoOrder)
    {
        auto runtimeIt = runtimeNodes.find(nodeId);
        if (runtimeIt == runtimeNodes.end())
        {
            continue;
        }

        EvaluateRuntimeNode(
            graph,
            *m_resources,
            edges,
            runtimeNodes,
            runtimeIt->second,
            result,
            resourceSummaries
        );
    }

    EvaluateScenarioObjectives(*m_resources, scenario, resourceSummaries, result);
    FinalizeResourceSummaries(*m_resources, resourceSummaries, result);

    if (result.scenarioComplete)
    {
        result.events.push_back("Scenario complete");
    }
    else
    {
        result.events.push_back("Scenario incomplete");
    }

    return result;
}

ProductionEvaluator CreateTier0ProductionEvaluator()
{
    // Function-local statics avoid global initialization order problems while
    // providing stable catalog lifetimes for this convenience evaluator.
    static const ResourceCatalog resources;
    static const MachineCatalog machines;
    static const RecipeCatalog recipes;
    return ProductionEvaluator(resources, machines, recipes);
}
} // namespace graph
