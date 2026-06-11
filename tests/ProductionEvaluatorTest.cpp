// Verifies the MVP production evaluator against the Tier 0 recipe-chain puzzle.
// These tests cover scenario completion, byproduct handling, input starvation,
// metadata-driven meaning, and MVP one-edge-per-port rules before the UI starts
// consuming production evaluation results.

#include "graph/GraphDocument.h"
#include "graph/ProductionEvaluator.h"
#include "graph/SampleGraph.h"
#include "production/ProductionGraphMetadata.h"

#include <cmath>
#include <iostream>

namespace
{
constexpr float Epsilon = 0.0001f;

bool Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Production evaluator test failed: " << message << "\n";
        return false;
    }

    return true;
}

bool CheckNear(float actual, float expected, const char* message)
{
    if (std::fabs(actual - expected) > Epsilon)
    {
        std::cerr
            << "Production evaluator test failed: " << message
            << " expected=" << expected
            << " actual=" << actual
            << "\n";
        return false;
    }

    return true;
}

struct EvaluationFixture
{
    graph::ResourceCatalog resources;
    graph::MachineCatalog machines;
    graph::RecipeCatalog recipes;
    graph::ScenarioCatalog scenarios;
};

const graph::ScenarioDef& RequireTier0Scenario(const EvaluationFixture& fixture)
{
    const graph::ScenarioDef* scenario = fixture.scenarios.Find(graph::production_ids::Tier0IronIngotScenario);
    Check(scenario != nullptr, "Tier 0 scenario should exist");
    return *scenario;
}

const graph::ResourceFlowSummary* FindResourceSummary(
    const graph::ProductionEvaluation& evaluation,
    graph::ResourceId resourceId
)
{
    for (const graph::ResourceFlowSummary& summary : evaluation.resources)
    {
        if (summary.resourceId == resourceId)
        {
            return &summary;
        }
    }

    return nullptr;
}

const graph::ObjectiveStatus* FindObjectiveStatus(
    const graph::ProductionEvaluation& evaluation,
    graph::ObjectiveKind kind,
    graph::ResourceId resourceId
)
{
    for (const graph::ObjectiveStatus& status : evaluation.objectives)
    {
        if (status.kind == kind && status.resourceId == resourceId)
        {
            return &status;
        }
    }

    return nullptr;
}

const graph::NodeProductionEval* FindNodeEvaluation(
    const graph::ProductionEvaluation& evaluation,
    int nodeId
)
{
    for (const graph::NodeProductionEval& node : evaluation.nodes)
    {
        if (node.nodeId == nodeId)
        {
            return &node;
        }
    }

    return nullptr;
}

const graph::GraphNode* FindNodeByRecipe(
    const graph::GraphDocument& graph,
    graph::RecipeId recipeId
)
{
    for (const graph::GraphNode& node : graph.nodes)
    {
        if (node.recipeId == recipeId)
        {
            return &node;
        }
    }

    return nullptr;
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

void RemoveEdgeToResourceInput(
    graph::GraphDocument& graph,
    int nodeId,
    graph::ResourceId resourceId
)
{
    const graph::GraphNode* node = graph::FindNode(graph, nodeId);
    if (node == nullptr)
    {
        return;
    }

    const graph::GraphPort* input = FindInputByProductionResource(*node, resourceId);
    if (input == nullptr)
    {
        return;
    }

    for (const graph::GraphEdge& edge : graph.edges)
    {
        if (edge.toNodeId == nodeId && edge.toPortId == input->id)
        {
            graph::RemoveEdge(graph, edge.id);
            return;
        }
    }
}

void StripEmbeddedProductionMeaning(graph::GraphDocument& graph)
{
    // Regression guard for the metadata migration: evaluator tests can remove
    // mirrored machine/recipe/resource fields from the graph and still pass when
    // ProductionGraphMetadata is the source of Crushline meaning.
    for (graph::GraphNode& node : graph.nodes)
    {
        node.machineId = graph::InvalidMachineId;
        node.recipeId = graph::InvalidRecipeId;

        for (graph::GraphPort& port : node.inputs)
        {
            port.productionResourceId = graph::InvalidResourceId;
            port.isByproduct = false;
        }

        for (graph::GraphPort& port : node.outputs)
        {
            port.productionResourceId = graph::InvalidResourceId;
            port.isByproduct = false;
        }
    }
}

graph::ProductionEvaluation EvaluateFromMetadata(
    const EvaluationFixture& fixture,
    const graph::GraphDocument& graph
)
{
    const production::ProductionGraphMetadata metadata =
        graph::BuildProductionGraphMetadata(graph, fixture.recipes);

    graph::ProductionEvaluator evaluator(fixture.resources, fixture.machines, fixture.recipes);
    return evaluator.Evaluate(graph, metadata, RequireTier0Scenario(fixture));
}

bool CheckTier0SampleCompletesScenario()
{
    // The full recipe-driven sample graph should satisfy both Tier 0 objectives:
    // produce 50 Iron Ingots/min and route all Iron Slurry to the Waste Sink.
    // The graph copy has embedded production IDs stripped so this test proves
    // the evaluator consumes ProductionGraphMetadata for game meaning.
    EvaluationFixture fixture;
    graph::GraphDocument graph = graph::CreateSampleFactoryGraph();
    const production::ProductionGraphMetadata metadata =
        graph::BuildProductionGraphMetadata(graph, fixture.recipes);
    graph::GraphDocument structureOnlyGraph = graph;
    StripEmbeddedProductionMeaning(structureOnlyGraph);

    graph::ProductionEvaluator evaluator(fixture.resources, fixture.machines, fixture.recipes);
    const graph::ProductionEvaluation evaluation =
        evaluator.Evaluate(structureOnlyGraph, metadata, RequireTier0Scenario(fixture));

    const graph::ResourceFlowSummary* ironIngot = FindResourceSummary(evaluation, graph::production_ids::IronIngot);
    const graph::ResourceFlowSummary* ironSlurry = FindResourceSummary(evaluation, graph::production_ids::IronSlurry);
    const graph::ResourceFlowSummary* waste = FindResourceSummary(evaluation, graph::production_ids::Waste);
    const graph::ObjectiveStatus* ingotObjective = FindObjectiveStatus(
        evaluation,
        graph::ObjectiveKind::ProduceAtLeastRate,
        graph::production_ids::IronIngot
    );
    const graph::ObjectiveStatus* slurryObjective = FindObjectiveStatus(
        evaluation,
        graph::ObjectiveKind::HandleAllProduced,
        graph::production_ids::IronSlurry
    );

    return
        Check(evaluation.scenarioComplete, "Tier 0 sample should complete scenario") &&
        Check(!evaluation.hasCycle, "Tier 0 sample should be acyclic") &&
        Check(evaluation.invalidConnectionCount == 0, "Tier 0 sample should have valid production links") &&
        Check(evaluation.objectives.size() == 2, "Tier 0 scenario should produce two objective statuses") &&
        Check(ingotObjective != nullptr, "Iron Ingot objective should be reported") &&
        Check(slurryObjective != nullptr, "Iron Slurry handling objective should be reported") &&
        Check(ingotObjective->satisfied, "Iron Ingot objective should be satisfied") &&
        Check(slurryObjective->satisfied, "Iron Slurry handling objective should be satisfied") &&
        CheckNear(evaluation.targetSatisfactionRatio, 1.0f, "sample target satisfaction mismatch") &&
        CheckNear(evaluation.totalPowerKw, 280.0f, "sample power use should sum active recipe power") &&
        CheckNear(evaluation.totalWastePerMinute, 10.0f, "sample waste production mismatch") &&
        Check(ironIngot != nullptr, "Iron Ingot summary should exist") &&
        CheckNear(ironIngot->producedPerMinute, 50.0f, "Iron Ingot production mismatch") &&
        Check(ironSlurry != nullptr, "Iron Slurry summary should exist") &&
        CheckNear(ironSlurry->producedPerMinute, 10.0f, "Iron Slurry production mismatch") &&
        CheckNear(ironSlurry->consumedPerMinute, 10.0f, "Iron Slurry handling mismatch") &&
        CheckNear(ironSlurry->surplusPerMinute, 0.0f, "Iron Slurry should not be unmanaged") &&
        Check(waste != nullptr, "Waste summary should exist") &&
        CheckNear(waste->producedPerMinute, 10.0f, "Waste production mismatch");
}

bool CheckUnmanagedByproductFailsScenario()
{
    // Removing the slurry sink edge should leave the Iron Ingot target satisfied
    // while failing the byproduct-handling objective.
    EvaluationFixture fixture;
    graph::GraphDocument graph = graph::CreateSampleFactoryGraph();

    const graph::GraphNode* wasteSink = FindNodeByRecipe(graph, graph::production_ids::StoreIronSlurry);
    if (!Check(wasteSink != nullptr, "Waste Sink recipe node should exist"))
    {
        return false;
    }

    RemoveEdgeToResourceInput(graph, wasteSink->id, graph::production_ids::IronSlurry);

    const graph::ProductionEvaluation evaluation = EvaluateFromMetadata(fixture, graph);

    const graph::ObjectiveStatus* ingotObjective = FindObjectiveStatus(
        evaluation,
        graph::ObjectiveKind::ProduceAtLeastRate,
        graph::production_ids::IronIngot
    );
    const graph::ObjectiveStatus* slurryObjective = FindObjectiveStatus(
        evaluation,
        graph::ObjectiveKind::HandleAllProduced,
        graph::production_ids::IronSlurry
    );
    const graph::ResourceFlowSummary* ironSlurry = FindResourceSummary(evaluation, graph::production_ids::IronSlurry);

    return
        Check(!evaluation.scenarioComplete, "unmanaged slurry should fail scenario completion") &&
        Check(ingotObjective != nullptr && ingotObjective->satisfied, "Iron Ingot objective should remain satisfied") &&
        Check(slurryObjective != nullptr && !slurryObjective->satisfied, "Iron Slurry handling should fail") &&
        Check(ironSlurry != nullptr, "Iron Slurry summary should exist after sink removal") &&
        CheckNear(ironSlurry->producedPerMinute, 10.0f, "unmanaged slurry production mismatch") &&
        CheckNear(ironSlurry->consumedPerMinute, 0.0f, "unmanaged slurry should not be consumed") &&
        CheckNear(ironSlurry->surplusPerMinute, 10.0f, "unmanaged slurry surplus mismatch");
}

bool CheckMissingInputLimitsDownstreamProduction()
{
    // Removing water from the washer should starve the washer and all downstream
    // recipe nodes, causing the production objective to fail deterministically.
    EvaluationFixture fixture;
    graph::GraphDocument graph = graph::CreateSampleFactoryGraph();

    const graph::GraphNode* washer = FindNodeByRecipe(graph, graph::production_ids::WashCrushedIronOre);
    if (!Check(washer != nullptr, "Washer recipe node should exist"))
    {
        return false;
    }

    RemoveEdgeToResourceInput(graph, washer->id, graph::production_ids::Water);

    const graph::ProductionEvaluation evaluation = EvaluateFromMetadata(fixture, graph);
    const graph::NodeProductionEval* washerEval = FindNodeEvaluation(evaluation, washer->id);
    const graph::ObjectiveStatus* ingotObjective = FindObjectiveStatus(
        evaluation,
        graph::ObjectiveKind::ProduceAtLeastRate,
        graph::production_ids::IronIngot
    );
    const graph::ResourceFlowSummary* ironIngot = FindResourceSummary(evaluation, graph::production_ids::IronIngot);

    return
        Check(!evaluation.scenarioComplete, "missing water should fail scenario completion") &&
        Check(washerEval != nullptr, "Washer evaluation should exist") &&
        Check(washerEval->starved, "Washer should be starved without water") &&
        CheckNear(washerEval->utilization, 0.0f, "Washer utilization should be zero without water") &&
        Check(ingotObjective != nullptr && !ingotObjective->satisfied, "Iron Ingot objective should fail") &&
        Check(ironIngot != nullptr, "Iron Ingot summary should exist") &&
        CheckNear(ironIngot->producedPerMinute, 0.0f, "Iron Ingot should not be produced without water");
}

bool CheckSplitOutputInvalidatesScenario()
{
    // The editor can still create some links the production MVP disallows. The
    // evaluator must enforce one outgoing edge per output port until explicit
    // splitter machines exist.
    EvaluationFixture fixture;
    graph::GraphDocument graph = graph::CreateSampleFactoryGraph();

    const graph::GraphNode* smelter = FindNodeByRecipe(graph, graph::production_ids::SmeltWashedIronOre);
    if (!Check(smelter != nullptr, "Smelter recipe node should exist"))
    {
        return false;
    }

    const graph::GraphPort* ingotOutput = FindOutputByProductionResource(*smelter, graph::production_ids::IronIngot);
    if (!Check(ingotOutput != nullptr, "Smelter Iron Ingot output should exist"))
    {
        return false;
    }

    const int alternateTarget = graph::AddNode(graph, graph::NodeType::Output, "Alternate Iron Ingot Target");
    const int alternateInput = graph::AddInputPort(
        graph,
        alternateTarget,
        "Iron Ingot",
        graph::ResourceType::IronIngot,
        graph::production_ids::IronIngot
    );

    // This is valid for the current editor helper but invalid for production MVP
    // cardinality, giving the evaluator a focused regression case.
    const int extraEdge = graph::AddEdge(graph, smelter->id, ingotOutput->id, alternateTarget, alternateInput);
    if (!Check(extraEdge != 0, "setup should create second outgoing edge before evaluator rejects split"))
    {
        return false;
    }

    const graph::ProductionEvaluation evaluation = EvaluateFromMetadata(fixture, graph);

    return
        Check(!evaluation.scenarioComplete, "implicit output split should fail scenario completion") &&
        Check(evaluation.invalidConnectionCount > 0, "implicit output split should be counted invalid");
}
} // namespace

int RunProductionEvaluatorTest()
{
    if (!CheckTier0SampleCompletesScenario() ||
        !CheckUnmanagedByproductFailsScenario() ||
        !CheckMissingInputLimitsDownstreamProduction() ||
        !CheckSplitOutputInvalidatesScenario())
    {
        return 1;
    }

    std::cout << "Production evaluator test passed.\n";
    return 0;
}
