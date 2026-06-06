#pragma once

#include "graph/GraphTypes.h"

#include <string>
#include <vector>

namespace graph
{

struct NodeEvalResult
{
    int nodeId = 0;

    float inputRate = 0.0f;
    float outputRate = 0.0f;
    float utilization = 0.0f;
    float efficiency = 1.0f;
    float powerUse = 0.0f;

    bool bottleneck = false;
    bool warning = false;
    std::string warningText{};
};

struct SimulationResult
{
    float totalThroughput = 0.0f;
    float totalPowerUse = 0.0f;
    float totalPowerCapacity = 600.0f;
    float plantEfficiency = 1.0f;
    float wasteStoragePercent = 0.0f;

    int warningCount = 0;
    int bottleneckCount = 0;

    std::vector<NodeEvalResult> nodes{};
    std::vector<std::string> events{};
};

SimulationResult EvaluateGraph(const GraphDocument& graph);

} // namespace graph
