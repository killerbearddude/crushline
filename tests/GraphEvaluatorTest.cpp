#include "graph/GraphEvaluator.h"

#include "graph/GraphDocument.h"
#include "graph/SampleGraph.h"

#include <cmath>
#include <iostream>
#include <string>

namespace
{
bool NearlyEqual(float a, float b)
{
    return std::fabs(a - b) <= 0.0001f;
}

bool Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Graph evaluator test failed: " << message << "\n";
        return false;
    }

    return true;
}

bool CheckNear(float actual, float expected, const char* message)
{
    if (!NearlyEqual(actual, expected))
    {
        std::cerr
            << "Graph evaluator test failed: " << message
            << " expected=" << expected
            << " actual=" << actual
            << "\n";
        return false;
    }

    return true;
}

const graph::NodeEvalResult* FindNodeResult(const graph::SimulationResult& result, int nodeId)
{
    for (const graph::NodeEvalResult& node : result.nodes)
    {
        if (node.nodeId == nodeId)
        {
            return &node;
        }
    }

    return nullptr;
}

const graph::GraphNode* FindNodeByName(const graph::GraphDocument& graph, const std::string& name)
{
    for (const graph::GraphNode& node : graph.nodes)
    {
        if (node.name == name)
        {
            return &node;
        }
    }

    return nullptr;
}

bool ContainsEvent(const graph::SimulationResult& result, const std::string& text)
{
    for (const std::string& event : result.events)
    {
        if (event == text)
        {
            return true;
        }
    }

    return false;
}

bool CheckSampleGraphMetrics()
{
    const graph::GraphDocument graph = graph::CreateSampleFactoryGraph();
    const graph::SimulationResult result = graph::EvaluateGraph(graph);

    const graph::GraphNode* washer = FindNodeByName(graph, "Washer");
    const graph::GraphNode* wasteStorage = FindNodeByName(graph, "Waste Storage");

    if (!Check(washer != nullptr, "sample graph should contain Washer") ||
        !Check(wasteStorage != nullptr, "sample graph should contain Waste Storage") ||
        !Check(result.nodes.size() == graph.nodes.size(), "node result count should match graph node count") ||
        !CheckNear(result.totalThroughput, 88.0f, "sample total throughput mismatch") ||
        !CheckNear(result.totalPowerUse, 540.0f, "sample total power use mismatch") ||
        !CheckNear(result.totalPowerCapacity, 600.0f, "sample power capacity mismatch") ||
        !CheckNear(result.plantEfficiency, 0.9357143f, "sample plant efficiency mismatch") ||
        !CheckNear(result.wasteStoragePercent, 23.0f / 25.0f, "sample waste storage percent mismatch") ||
        !Check(result.warningCount == 3, "sample warning count should include low power reserve") ||
        !Check(result.bottleneckCount == 0, "sample bottleneck count should be zero") ||
        !Check(ContainsEvent(result, "Low power reserve"), "sample should report low power reserve"))
    {
        return false;
    }

    const graph::NodeEvalResult* washerResult = FindNodeResult(result, washer->id);
    const graph::NodeEvalResult* wasteResult = FindNodeResult(result, wasteStorage->id);

    return
        Check(washerResult != nullptr, "missing Washer evaluation result") &&
        Check(washerResult->warning, "Washer warning should propagate") &&
        Check(washerResult->warningText == "Clogged by high slurry content", "Washer warning text should propagate") &&
        CheckNear(washerResult->utilization, 108.0f / 120.0f, "Washer utilization mismatch") &&
        CheckNear(washerResult->outputRate, 108.0f * 0.90f, "Washer output rate mismatch") &&
        Check(wasteResult != nullptr, "missing Waste Storage evaluation result") &&
        Check(wasteResult->warning, "Waste Storage warning should propagate") &&
        CheckNear(wasteResult->utilization, 23.0f / 25.0f, "Waste Storage utilization mismatch");
}

bool CheckBottleneckDetection()
{
    graph::GraphDocument graph;

    const int overloaded = graph::AddNode(graph, graph::NodeType::Machine, "Overloaded Crusher");
    if (graph::GraphNode* node = graph::FindNode(graph, overloaded))
    {
        node->throughput = 150.0f;
        node->capacity = 100.0f;
        node->efficiency = 0.80f;
        node->powerUse = 80.0f;
    }

    const graph::SimulationResult result = graph::EvaluateGraph(graph);
    const graph::NodeEvalResult* nodeResult = FindNodeResult(result, overloaded);

    return
        Check(result.nodes.size() == 1, "single-node graph should produce one node result") &&
        CheckNear(result.totalThroughput, 150.0f, "fallback throughput should use node throughput") &&
        Check(result.warningCount == 1, "over-capacity node should count as warning") &&
        Check(result.bottleneckCount == 1, "over-capacity node should count as bottleneck") &&
        Check(ContainsEvent(result, "One or more nodes are bottlenecked"), "bottleneck event should be emitted") &&
        Check(nodeResult != nullptr, "missing overloaded node result") &&
        Check(nodeResult->warning, "overloaded node should warn") &&
        Check(nodeResult->bottleneck, "overloaded node should be bottlenecked") &&
        Check(nodeResult->warningText == "Throughput exceeds capacity", "overloaded warning text mismatch") &&
        CheckNear(nodeResult->utilization, 1.5f, "overloaded utilization mismatch");
}

bool CheckPowerCapacityEvent()
{
    graph::GraphDocument graph;

    const int furnace = graph::AddNode(graph, graph::NodeType::Machine, "Emergency Furnace");
    if (graph::GraphNode* node = graph::FindNode(graph, furnace))
    {
        node->throughput = 25.0f;
        node->capacity = 50.0f;
        node->efficiency = 1.0f;
        node->powerUse = 700.0f;
    }

    const graph::SimulationResult result = graph::EvaluateGraph(graph);

    return
        CheckNear(result.totalPowerUse, 700.0f, "power use mismatch") &&
        Check(result.warningCount == 1, "power capacity breach should add one warning") &&
        Check(result.bottleneckCount == 0, "power capacity breach should not create node bottleneck") &&
        Check(ContainsEvent(result, "Power demand exceeds available capacity"), "power capacity event should be emitted");
}

bool CheckRemoveNodeEvaluation()
{
    graph::GraphDocument graph = graph::CreateSampleFactoryGraph();
    const graph::GraphNode* washer = FindNodeByName(graph, "Washer");

    if (!Check(washer != nullptr, "sample graph should contain Washer before removal"))
    {
        return false;
    }

    const int removedNodeId = washer->id;
    graph::RemoveNode(graph, removedNodeId);

    const graph::SimulationResult result = graph::EvaluateGraph(graph);

    return
        Check(result.nodes.size() == graph.nodes.size(), "result count should match graph after removal") &&
        Check(graph.edges.size() == 3, "removing Washer should remove its connected edges") &&
        Check(result.warningCount == 1, "removing Washer should leave only Waste Storage warning") &&
        Check(result.bottleneckCount == 0, "removing Washer should not create bottlenecks") &&
        Check(FindNodeResult(result, removedNodeId) == nullptr, "removed node should not have an evaluation result");
}
}

int RunGraphEvaluatorTest()
{
    if (!CheckSampleGraphMetrics() ||
        !CheckBottleneckDetection() ||
        !CheckPowerCapacityEvent() ||
        !CheckRemoveNodeEvaluation())
    {
        return 1;
    }

    std::cout << "Graph evaluator test passed.\n";
    return 0;
}
