#pragma once

#include "graph/GraphTypes.h"

#include <string>

namespace graph
{

GraphNode* FindNode(GraphDocument& graph, int nodeId);
const GraphNode* FindNode(const GraphDocument& graph, int nodeId);

GraphPort* FindPort(GraphNode& node, int portId);
const GraphPort* FindPort(const GraphNode& node, int portId);
const GraphPort* FindPort(const GraphDocument& graph, int nodeId, int portId);

int AddNode(GraphDocument& graph, NodeType type, std::string name);
int AddInputPort(GraphDocument& graph, int nodeId, std::string name, ResourceType resource);
int AddOutputPort(GraphDocument& graph, int nodeId, std::string name, ResourceType resource);

bool CanConnect(
    const GraphDocument& graph,
    int fromNodeId,
    int fromPortId,
    int toNodeId,
    int toPortId
);

int AddEdge(
    GraphDocument& graph,
    int fromNodeId,
    int fromPortId,
    int toNodeId,
    int toPortId
);

void RemoveEdge(GraphDocument& graph, int edgeId);
void RemoveNode(GraphDocument& graph, int nodeId);

} // namespace graph
