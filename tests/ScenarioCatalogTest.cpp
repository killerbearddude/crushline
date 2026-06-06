// Verifies the Tier 0 scenario catalog and its typed objective model. These
// tests protect the design decision that scenario goals are not just production
// rates: byproduct handling is a separate objective kind that the evaluator will
// resolve in a later patch.

#include "graph/ResourceCatalog.h"
#include "graph/ScenarioCatalog.h"

#include <cmath>
#include <iostream>
#include <string_view>

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
        std::cerr << "Scenario catalog test failed: " << message << "\n";
        return false;
    }

    return true;
}

bool CheckNear(float actual, float expected, const char* message)
{
    if (!NearlyEqual(actual, expected))
    {
        std::cerr
            << "Scenario catalog test failed: " << message
            << " expected=" << expected
            << " actual=" << actual
            << "\n";
        return false;
    }

    return true;
}

const graph::ScenarioDef* RequireScenarioByKey(
    const graph::ScenarioCatalog& catalog,
    std::string_view key
)
{
    const graph::ScenarioDef* scenario = catalog.FindByKey(key);
    Check(scenario != nullptr, "Required scenario key should exist");
    return scenario;
}

bool CheckTier0ScenarioDefinition()
{
    // Verifies the user-facing Tier 0 puzzle: produce ingots while handling the
    // slurry byproduct. This prevents future objective work from flattening all
    // goals into a single production target.
    const graph::ScenarioCatalog scenarios;

    const graph::ScenarioDef* scenario = RequireScenarioByKey(scenarios, "tier0_iron_ingot");
    if (!Check(scenario != nullptr, "Tier 0 scenario should be required by key"))
    {
        return false;
    }

    const graph::ScenarioObjective* ingotTarget = graph::FindObjective(
        *scenario,
        graph::ObjectiveKind::ProduceAtLeastRate,
        graph::production_ids::IronIngot
    );
    const graph::ScenarioObjective* slurryHandling = graph::FindObjective(
        *scenario,
        graph::ObjectiveKind::HandleAllProduced,
        graph::production_ids::IronSlurry
    );

    return
        Check(scenarios.All().size() == 1, "Tier 0 should expose one scenario") &&
        Check(scenario->id == graph::production_ids::Tier0IronIngotScenario, "Tier 0 scenario id mismatch") &&
        Check(scenario->tier == graph::TechTier::Tier0, "Tier 0 scenario tier mismatch") &&
        Check(scenario->displayName == "Tier 0: Iron Ingot Chain", "Tier 0 scenario display name mismatch") &&
        Check(scenario->objectives.size() == 2, "Tier 0 scenario should have two objectives") &&
        Check(ingotTarget != nullptr, "Tier 0 should require Iron Ingot production") &&
        CheckNear(ingotTarget->thresholdRatePerMinute, 50.0f, "Iron Ingot target rate mismatch") &&
        Check(slurryHandling != nullptr, "Tier 0 should require Iron Slurry handling") &&
        CheckNear(slurryHandling->thresholdRatePerMinute, 0.0f, "HandleAllProduced should not use a rate threshold") &&
        Check(scenarios.Find(graph::production_ids::Tier0IronIngotScenario) == scenario, "Scenario id lookup should return the same scenario") &&
        Check(scenarios.Find(99999) == nullptr, "Unknown scenario id lookup should return null") &&
        Check(scenarios.FindByKey("missing_scenario") == nullptr, "Unknown scenario key lookup should return null");
}

bool CheckScenarioObjectiveHelpers()
{
    // Verifies helper functions remain objective-kind aware. The evaluator will
    // use this separation to report target deficits independently from unmanaged
    // byproducts.
    const graph::ScenarioCatalog scenarios;
    const graph::ScenarioDef* scenario = RequireScenarioByKey(scenarios, "tier0_iron_ingot");
    if (!Check(scenario != nullptr, "Scenario helper test requires Tier 0 scenario"))
    {
        return false;
    }

    return
        Check(
            graph::ScenarioRequiresProductionAtLeast(*scenario, graph::production_ids::IronIngot, 50.0f),
            "Scenario should require 50 Iron Ingots per minute"
        ) &&
        Check(
            !graph::ScenarioRequiresProductionAtLeast(*scenario, graph::production_ids::IronIngot, 60.0f),
            "Scenario should not report the wrong Iron Ingot rate"
        ) &&
        Check(
            !graph::ScenarioRequiresProductionAtLeast(*scenario, graph::production_ids::IronSlurry, 50.0f),
            "Byproduct handling should not be mistaken for a production target"
        ) &&
        Check(
            graph::ScenarioRequiresHandlingAllProduced(*scenario, graph::production_ids::IronSlurry),
            "Scenario should require all Iron Slurry to be handled"
        ) &&
        Check(
            !graph::ScenarioRequiresHandlingAllProduced(*scenario, graph::production_ids::IronIngot),
            "Iron Ingot production should not be mistaken for byproduct handling"
        );
}

bool CheckScenarioResourceIntegrity()
{
    // Verifies that every scenario objective references a real resource. This
    // catches catalog drift before evaluator code tries to resolve an objective
    // for a missing resource definition.
    const graph::ResourceCatalog resources;
    const graph::ScenarioCatalog scenarios;

    for (const graph::ScenarioDef& scenario : scenarios.All())
    {
        if (!Check(scenario.id != graph::InvalidScenarioId, "Scenario id should be valid") ||
            !Check(!scenario.key.empty(), "Scenario key should be non-empty") ||
            !Check(!scenario.displayName.empty(), "Scenario display name should be non-empty") ||
            !Check(!scenario.objectives.empty(), "Scenario should contain at least one objective"))
        {
            return false;
        }

        for (const graph::ScenarioObjective& objective : scenario.objectives)
        {
            if (!Check(objective.resourceId != graph::InvalidResourceId, "Objective resource id should be valid") ||
                !Check(resources.Find(objective.resourceId) != nullptr, "Objective resource should exist"))
            {
                return false;
            }

            if (objective.kind == graph::ObjectiveKind::ProduceAtLeastRate &&
                !Check(objective.thresholdRatePerMinute > 0.0f, "Production objective rate should be positive"))
            {
                return false;
            }
        }
    }

    return true;
}
} // namespace

int RunScenarioCatalogTest()
{
    if (!CheckTier0ScenarioDefinition() ||
        !CheckScenarioObjectiveHelpers() ||
        !CheckScenarioResourceIntegrity())
    {
        return 1;
    }

    return 0;
}
