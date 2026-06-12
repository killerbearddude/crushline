#include "graph/GraphDocument.h"

#include <cstddef>
#include <iostream>

namespace
{
bool Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Graph connection test failed: " << message << "\n";
        return false;
    }

    return true;
}

struct TestPorts
{
    int sourceNodeId = 0;
    int machineNodeId = 0;
    int storageNodeId = 0;

    int sourceIronOutputId = 0;
    int sourceCoalOutputId = 0;
    int machineIronInputId = 0;
    int machineIronOutputId = 0;
    int storageIronInputId = 0;
    int storageCoalInputId = 0;
};

TestPorts BuildConnectionTestGraph(graph::GraphDocument& graph)
{
    TestPorts ports;

    ports.sourceNodeId = graph::AddNode(graph, graph::NodeType::Source, "Source");
    ports.machineNodeId = graph::AddNode(graph, graph::NodeType::Machine, "Machine");
    ports.storageNodeId = graph::AddNode(graph, graph::NodeType::Storage, "Storage");

    ports.sourceIronOutputId = graph::AddOutputPort(
        graph,
        ports.sourceNodeId,
        "Iron Ore",
        graph::ResourceType::IronOre
    );

    ports.sourceCoalOutputId = graph::AddOutputPort(
        graph,
        ports.sourceNodeId,
        "Coal",
        graph::ResourceType::Coal
    );

    ports.machineIronInputId = graph::AddInputPort(
        graph,
        ports.machineNodeId,
        "Iron Ore",
        graph::ResourceType::IronOre
    );

    ports.machineIronOutputId = graph::AddOutputPort(
        graph,
        ports.machineNodeId,
        "Crushed Ore",
        graph::ResourceType::CrushedOre
    );

    ports.storageIronInputId = graph::AddInputPort(
        graph,
        ports.storageNodeId,
        "Iron Ore",
        graph::ResourceType::IronOre
    );

    ports.storageCoalInputId = graph::AddInputPort(
        graph,
        ports.storageNodeId,
        "Coal",
        graph::ResourceType::Coal
    );

    return ports;
}

bool CheckValidOutputToInputConnection()
{
    graph::GraphDocument graph;
    const TestPorts ports = BuildConnectionTestGraph(graph);

    if (!Check(graph::CanConnect(
            graph,
            ports.sourceNodeId,
            ports.sourceIronOutputId,
            ports.machineNodeId,
            ports.machineIronInputId
        ), "matching output-to-input connection should be valid"))
    {
        return false;
    }

    const int edgeId = graph::AddEdge(
        graph,
        ports.sourceNodeId,
        ports.sourceIronOutputId,
        ports.machineNodeId,
        ports.machineIronInputId
    );

    return
        Check(edgeId != 0, "valid connection should create an edge") &&
        Check(graph.edges.size() == 1, "valid connection should append one edge") &&
        Check(graph.edges.front().id == edgeId, "created edge id should be stored");
}

bool CheckInputToInputRejected()
{
    graph::GraphDocument graph;
    const TestPorts ports = BuildConnectionTestGraph(graph);

    const std::size_t edgeCountBefore = graph.edges.size();
    const bool canConnect = graph::CanConnect(
        graph,
        ports.machineNodeId,
        ports.machineIronInputId,
        ports.storageNodeId,
        ports.storageIronInputId
    );

    const int edgeId = graph::AddEdge(
        graph,
        ports.machineNodeId,
        ports.machineIronInputId,
        ports.storageNodeId,
        ports.storageIronInputId
    );

    return
        Check(!canConnect, "input-to-input connection should be rejected") &&
        Check(edgeId == 0, "input-to-input add should return zero") &&
        Check(graph.edges.size() == edgeCountBefore, "input-to-input add should not mutate edges");
}

bool CheckOutputToOutputRejected()
{
    graph::GraphDocument graph;
    const TestPorts ports = BuildConnectionTestGraph(graph);

    const bool canConnect = graph::CanConnect(
        graph,
        ports.sourceNodeId,
        ports.sourceIronOutputId,
        ports.machineNodeId,
        ports.machineIronOutputId
    );

    const int edgeId = graph::AddEdge(
        graph,
        ports.sourceNodeId,
        ports.sourceIronOutputId,
        ports.machineNodeId,
        ports.machineIronOutputId
    );

    return
        Check(!canConnect, "output-to-output connection should be rejected") &&
        Check(edgeId == 0, "output-to-output add should return zero") &&
        Check(graph.edges.empty(), "output-to-output add should not mutate edges");
}

bool CheckSelfConnectionRejected()
{
    graph::GraphDocument graph;
    const int nodeId = graph::AddNode(graph, graph::NodeType::Machine, "Loopback Machine");
    const int outputId = graph::AddOutputPort(graph, nodeId, "Iron Ore Out", graph::ResourceType::IronOre);
    const int inputId = graph::AddInputPort(graph, nodeId, "Iron Ore In", graph::ResourceType::IronOre);

    const bool canConnect = graph::CanConnect(graph, nodeId, outputId, nodeId, inputId);
    const int edgeId = graph::AddEdge(graph, nodeId, outputId, nodeId, inputId);

    return
        Check(!canConnect, "node self-connection should be rejected") &&
        Check(edgeId == 0, "self-connection add should return zero") &&
        Check(graph.edges.empty(), "self-connection add should not mutate edges");
}

bool CheckDuplicateEdgeRejected()
{
    graph::GraphDocument graph;
    const TestPorts ports = BuildConnectionTestGraph(graph);

    const int firstEdgeId = graph::AddEdge(
        graph,
        ports.sourceNodeId,
        ports.sourceIronOutputId,
        ports.machineNodeId,
        ports.machineIronInputId
    );

    if (!Check(firstEdgeId != 0, "first edge should be created"))
    {
        return false;
    }

    const bool secondCanConnect = graph::CanConnect(
        graph,
        ports.sourceNodeId,
        ports.sourceIronOutputId,
        ports.machineNodeId,
        ports.machineIronInputId
    );

    const int secondEdgeId = graph::AddEdge(
        graph,
        ports.sourceNodeId,
        ports.sourceIronOutputId,
        ports.machineNodeId,
        ports.machineIronInputId
    );

    return
        Check(!secondCanConnect, "duplicate edge should be rejected by CanConnect") &&
        Check(secondEdgeId == 0, "duplicate AddEdge should return zero") &&
        Check(graph.edges.size() == 1, "duplicate AddEdge should not append an edge") &&
        Check(graph.nextEdgeId == firstEdgeId + 1, "duplicate AddEdge should not consume an edge id");
}

bool CheckResourceMismatchAllowedAsSoftFailure()
{
    // Wrong-resource links are playable production mistakes, not hard graph
    // structure errors. This prevents the editor from blocking experimentation;
    // the production evaluator owns the zero-flow/diagnostic consequence.
    graph::GraphDocument graph;
    const TestPorts ports = BuildConnectionTestGraph(graph);

    const bool ironOutputToCoalInput = graph::CanConnect(
        graph,
        ports.sourceNodeId,
        ports.sourceIronOutputId,
        ports.storageNodeId,
        ports.storageCoalInputId
    );

    const int edgeId = graph::AddEdge(
        graph,
        ports.sourceNodeId,
        ports.sourceIronOutputId,
        ports.storageNodeId,
        ports.storageCoalInputId
    );

    return
        Check(ironOutputToCoalInput, "resource mismatch should be allowed as a soft failure") &&
        Check(edgeId != 0, "resource mismatch AddEdge should create an editable edge") &&
        Check(graph.edges.size() == 1, "resource mismatch AddEdge should append one edge") &&
        Check(graph.edges.front().fromNodeId == ports.sourceNodeId, "mismatch edge source node mismatch") &&
        Check(graph.edges.front().fromPortId == ports.sourceIronOutputId, "mismatch edge source port mismatch") &&
        Check(graph.edges.front().toNodeId == ports.storageNodeId, "mismatch edge target node mismatch") &&
        Check(graph.edges.front().toPortId == ports.storageCoalInputId, "mismatch edge target port mismatch");
}
}

int RunGraphConnectionTest()
{
    if (!CheckValidOutputToInputConnection() ||
        !CheckInputToInputRejected() ||
        !CheckOutputToOutputRejected() ||
        !CheckSelfConnectionRejected() ||
        !CheckDuplicateEdgeRejected() ||
        !CheckResourceMismatchAllowedAsSoftFailure())
    {
        return 1;
    }

    std::cout << "Graph connection test passed.\n";
    return 0;
}
