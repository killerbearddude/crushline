#pragma once

// Declares scenario objective data for the production puzzle loop. Scenarios are
// gameplay definitions that reference production catalog resources but do not
// inspect graph topology or solve production flow; evaluator patches consume
// these objectives later to decide whether a graph satisfies the scenario.

#include "graph/ProductionTypes.h"

#include <string_view>
#include <vector>

namespace graph
{
class ScenarioCatalog
{
public:
    // Builds the current hardcoded Tier 0 scenario set.
    ScenarioCatalog();

    // Returns every scenario definition owned by this catalog. The returned
    // reference remains valid for the lifetime of the catalog instance.
    [[nodiscard]] const std::vector<ScenarioDef>& All() const;

    // Finds a scenario by stable catalog ID. The returned pointer is non-owning
    // and remains valid until this catalog is destroyed; returns nullptr when
    // the ID is unknown.
    [[nodiscard]] const ScenarioDef* Find(ScenarioId id) const;

    // Finds a scenario by content key, such as "tier0_iron_ingot". The returned
    // pointer is non-owning and remains valid until this catalog is destroyed;
    // returns nullptr when the key is unknown.
    [[nodiscard]] const ScenarioDef* FindByKey(std::string_view key) const;

private:
    std::vector<ScenarioDef> m_scenarios;
};

// Finds the first objective matching both kind and resource. Returns nullptr
// when the scenario does not contain that typed condition.
[[nodiscard]] const ScenarioObjective* FindObjective(
    const ScenarioDef& scenario,
    ObjectiveKind kind,
    ResourceId resourceId
);

// Returns true when the scenario requires the resource to be produced at or
// above a rate. This helper is intentionally objective-type aware so callers do
// not confuse production targets with byproduct-handling requirements.
[[nodiscard]] bool ScenarioRequiresProductionAtLeast(
    const ScenarioDef& scenario,
    ResourceId resourceId,
    float requiredRatePerMinute
);

// Returns true when the scenario requires all produced instances of this
// resource to be consumed or routed. The future evaluator will apply this to
// byproduct resources such as Iron Slurry.
[[nodiscard]] bool ScenarioRequiresHandlingAllProduced(
    const ScenarioDef& scenario,
    ResourceId resourceId
);

// Creates the current Tier 0 scenario catalog. This mirrors the production
// catalog factories and leaves room for later data-file-backed construction.
[[nodiscard]] ScenarioCatalog CreateTier0ScenarioCatalog();
} // namespace graph
