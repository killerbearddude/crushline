#include "graph/GraphEvaluator.h"

#include <algorithm>
#include <utility>

namespace graph
{
namespace
{
float SafeUtilization(float throughput, float capacity)
{
    if (capacity <= 0.0f)
    {
        return 0.0f;
    }

    return throughput / capacity;
}

bool IsOutputLike(NodeType type)
{
    return type == NodeType::Output || type == NodeType::Contract;
}

bool IsWasteStorageNode(const GraphNode& node)
{
    return node.type == NodeType::Storage &&
           node.name.find("Waste") != std::string::npos;
}
}

SimulationResult EvaluateGraph(const GraphDocument& graph)
{
    SimulationResult result;
    result.nodes.reserve(graph.nodes.size());

    float efficiencySum = 0.0f;
    int efficiencyCount = 0;

    float outputThroughput = 0.0f;
    float fallbackThroughput = 0.0f;

    float highestWasteStoragePercent = 0.0f;

    for (const GraphNode& node : graph.nodes)
    {
        NodeEvalResult nodeResult;
        nodeResult.nodeId = node.id;
        nodeResult.inputRate = node.throughput;
        nodeResult.outputRate = node.throughput * node.efficiency;
        nodeResult.utilization = SafeUtilization(node.throughput, node.capacity);
        nodeResult.efficiency = node.efficiency;
        nodeResult.powerUse = node.powerUse;
        nodeResult.warning = node.warning;
        nodeResult.warningText = node.warningText;

        if (node.capacity > 0.0f && node.throughput > node.capacity)
        {
            nodeResult.bottleneck = true;
            nodeResult.warning = true;
            nodeResult.warningText = "Throughput exceeds capacity";
        }

        if (nodeResult.warning)
        {
            ++result.warningCount;
        }

        if (nodeResult.bottleneck)
        {
            ++result.bottleneckCount;
        }

        if (IsOutputLike(node.type))
        {
            outputThroughput += node.throughput;
        }

        fallbackThroughput += node.throughput;
        result.totalPowerUse += node.powerUse;

        if (node.efficiency > 0.0f)
        {
            efficiencySum += node.efficiency;
            ++efficiencyCount;
        }

        if (IsWasteStorageNode(node))
        {
            highestWasteStoragePercent = std::max(
                highestWasteStoragePercent,
                SafeUtilization(node.throughput, node.capacity)
            );
        }

        result.nodes.push_back(std::move(nodeResult));
    }

    result.totalThroughput = outputThroughput > 0.0f ? outputThroughput : fallbackThroughput;

    if (efficiencyCount > 0)
    {
        result.plantEfficiency = efficiencySum / static_cast<float>(efficiencyCount);
    }

    result.wasteStoragePercent = highestWasteStoragePercent;

    if (result.totalPowerUse > result.totalPowerCapacity)
    {
        ++result.warningCount;
        result.events.push_back("Power demand exceeds available capacity");
    }
    else if (result.totalPowerCapacity > 0.0f)
    {
        const float reserve = result.totalPowerCapacity - result.totalPowerUse;
        const float reservePercent = reserve / result.totalPowerCapacity;

        if (reservePercent < 0.15f)
        {
            ++result.warningCount;
            result.events.push_back("Low power reserve");
        }
    }

    if (result.bottleneckCount > 0)
    {
        result.events.push_back("One or more nodes are bottlenecked");
    }

    return result;
}

} // namespace graph
