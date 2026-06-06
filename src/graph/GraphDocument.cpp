#include "graph/GraphDocument.h"

#include <algorithm>
#include <utility>

namespace graph
{
namespace
{
GraphPort MakePort(int id, std::string name, ResourceType resource, PortDirection direction)
{
    return GraphPort{
        .id = id,
        .name = std::move(name),
        .resource = resource,
        .direction = direction
    };
}
}

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

int AddInputPort(GraphDocument& graph, int nodeId, std::string name, ResourceType resource)
{
    GraphNode* node = FindNode(graph, nodeId);

    if (node == nullptr)
    {
        return 0;
    }

    const int id = graph.nextPortId++;
    node->inputs.push_back(MakePort(id, std::move(name), resource, PortDirection::Input));
    return id;
}

int AddOutputPort(GraphDocument& graph, int nodeId, std::string name, ResourceType resource)
{
    GraphNode* node = FindNode(graph, nodeId);

    if (node == nullptr)
    {
        return 0;
    }

    const int id = graph.nextPortId++;
    node->outputs.push_back(MakePort(id, std::move(name), resource, PortDirection::Output));
    return id;
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

    if (fromPort->resource != toPort->resource)
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

    graph.edges.erase(
        std::remove_if(graph.edges.begin(), graph.edges.end(), [nodeId](const GraphEdge& edge) {
            return edge.fromNodeId == nodeId || edge.toNodeId == nodeId;
        }),
        graph.edges.end()
    );
}

} // namespace graph
