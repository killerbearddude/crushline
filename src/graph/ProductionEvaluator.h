#pragma once

// Declares the MVP production evaluator for recipe-driven graphs. This system
// interprets graph topology through Crushline-owned production metadata,
// production catalogs, and scenario objectives.

#include "graph/GraphTypes.h"
#include "graph/MachineCatalog.h"
#include "graph/RecipeCatalog.h"
#include "graph/ResourceCatalog.h"
#include "graph/ScenarioCatalog.h"
#include "production/ProductionGraphMetadata.h"

#include <string>
#include <vector>

namespace graph
{
// Aggregated accounting for one production resource across the evaluated graph.
// Rates are per minute and are derived from recipe outputs, byproducts, and
// actual machine utilization.
struct ResourceFlowSummary
{
    ResourceId resourceId = InvalidResourceId;
    float producedPerMinute = 0.0f;
    float consumedPerMinute = 0.0f;
    float surplusPerMinute = 0.0f;
    float deficitPerMinute = 0.0f;
};

// Result for one typed scenario objective. ProduceAtLeastRate uses the scenario
// threshold as requiredPerMinute; HandleAllProduced uses the produced byproduct
// rate as the required handled amount for this evaluation pass.
struct ObjectiveStatus
{
    ObjectiveKind kind = ObjectiveKind::ProduceAtLeastRate;
    ResourceId resourceId = InvalidResourceId;
    float requiredPerMinute = 0.0f;
    float actualPerMinute = 0.0f;
    float satisfactionRatio = 0.0f;
    bool satisfied = false;
    std::string message{};
};

// Per-node production state after rate propagation. This is intentionally
// separate from the legacy NodeEvalResult so the UI can migrate gradually.
struct NodeProductionEval
{
    int nodeId = 0;
    bool active = false;
    bool starved = false;
    bool bottleneck = false;
    float utilization = 0.0f;
    std::vector<std::string> messages{};
};

// Complete production-solver output for one graph/scenario evaluation. The MVP
// evaluator is deterministic and conservative: invalid links or cycles prevent
// scenario completion even when resource rates would otherwise satisfy targets.
struct ProductionEvaluation
{
    std::vector<ResourceFlowSummary> resources{};
    std::vector<ObjectiveStatus> objectives{};
    std::vector<NodeProductionEval> nodes{};
    std::vector<std::string> events{};

    float totalPowerKw = 0.0f;
    float totalWastePerMinute = 0.0f;
    float targetSatisfactionRatio = 0.0f;

    int bottleneckCount = 0;
    int invalidConnectionCount = 0;

    bool hasCycle = false;
    bool scenarioComplete = false;
};

// Builds a metadata snapshot from the current legacy graph production fields.
// NOTE: This compatibility bridge exists while GraphDocument still mirrors
// Crushline production meaning directly on nodes and ports. The evaluator-facing
// API should consume the returned metadata rather than treating graph structure
// as the long-term owner of machine, recipe, resource, and port-role meaning.
[[nodiscard]] production::ProductionGraphMetadata BuildProductionGraphMetadata(
    const GraphDocument& graph,
    const RecipeCatalog& recipes
);

class ProductionEvaluator
{
public:
    // Creates a non-owning evaluator over stable catalog instances. The caller
    // must keep the catalogs alive for the evaluator lifetime.
    ProductionEvaluator(
        const ResourceCatalog& resources,
        const MachineCatalog& machines,
        const RecipeCatalog& recipes
    );

    // Evaluates one graph against one scenario using explicit Crushline
    // production metadata for machine, recipe, resource, port role, and link
    // meaning. GraphDocument remains the topology source: node IDs, port IDs,
    // edge IDs, and edge endpoints still come from graph structure.
    [[nodiscard]] ProductionEvaluation Evaluate(
        const GraphDocument& graph,
        const production::ProductionGraphMetadata& metadata,
        const ScenarioDef& scenario
    ) const;

    // Evaluates one graph against one scenario using the locked MVP rules:
    // acyclic topology, one edge per port, exact ResourceId matches, and
    // input-limited utilization. NOTE: Temporary compatibility overload for
    // legacy call sites that have not yet passed explicit production metadata.
    [[nodiscard]] ProductionEvaluation Evaluate(
        const GraphDocument& graph,
        const ScenarioDef& scenario
    ) const;

private:
    const ResourceCatalog* m_resources = nullptr;
    const MachineCatalog* m_machines = nullptr;
    const RecipeCatalog* m_recipes = nullptr;
};

// Convenience factory for the current hardcoded Tier 0 catalogs. Tests and early
// UI integration can use this when they do not need custom catalog instances.
[[nodiscard]] ProductionEvaluator CreateTier0ProductionEvaluator();
} // namespace graph
