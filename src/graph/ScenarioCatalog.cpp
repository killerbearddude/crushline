// Implements the hardcoded Tier 0 scenario catalog. Scenario definitions are
// intentionally small at this stage: they express objective conditions only and
// leave graph evaluation, scoring, and unlock progression to later systems.

#include "graph/ScenarioCatalog.h"

#include <cmath>

namespace graph
{
namespace
{
constexpr float RateComparisonEpsilon = 0.0001f;

std::vector<ScenarioDef> BuildTier0Scenarios()
{
    // NOTE: Tier 0 scenario data is hardcoded while the puzzle loop is being
    // proven. Once objective semantics stabilize, scenarios can move to external
    // data files alongside resources, machines, and recipes.
    return {
        {
            production_ids::Tier0IronIngotScenario,
            "tier0_iron_ingot",
            "Tier 0: Iron Ingot Chain",
            TechTier::Tier0,
            {
                {
                    ObjectiveKind::ProduceAtLeastRate,
                    production_ids::IronIngot,
                    50.0f,
                    0
                },
                {
                    ObjectiveKind::HandleAllProduced,
                    production_ids::IronSlurry,
                    0.0f,
                    0
                }
            }
        }
    };
}

bool NearlyEqual(float a, float b)
{
    return std::fabs(a - b) <= RateComparisonEpsilon;
}
} // namespace

ScenarioCatalog::ScenarioCatalog()
    : m_scenarios(BuildTier0Scenarios())
{
}

const std::vector<ScenarioDef>& ScenarioCatalog::All() const
{
    return m_scenarios;
}

const ScenarioDef* ScenarioCatalog::Find(ScenarioId id) const
{
    for (const ScenarioDef& scenario : m_scenarios)
    {
        if (scenario.id == id)
        {
            return &scenario;
        }
    }

    return nullptr;
}

const ScenarioDef* ScenarioCatalog::FindByKey(std::string_view key) const
{
    for (const ScenarioDef& scenario : m_scenarios)
    {
        if (scenario.key == key)
        {
            return &scenario;
        }
    }

    return nullptr;
}

const ScenarioObjective* FindObjective(
    const ScenarioDef& scenario,
    ObjectiveKind kind,
    ResourceId resourceId
)
{
    for (const ScenarioObjective& objective : scenario.objectives)
    {
        if (objective.kind == kind && objective.resourceId == resourceId)
        {
            return &objective;
        }
    }

    return nullptr;
}

bool ScenarioRequiresProductionAtLeast(
    const ScenarioDef& scenario,
    ResourceId resourceId,
    float requiredRatePerMinute
)
{
    const ScenarioObjective* objective = FindObjective(
        scenario,
        ObjectiveKind::ProduceAtLeastRate,
        resourceId
    );

    return objective != nullptr &&
           NearlyEqual(objective->thresholdRatePerMinute, requiredRatePerMinute);
}

bool ScenarioRequiresHandlingAllProduced(
    const ScenarioDef& scenario,
    ResourceId resourceId
)
{
    return FindObjective(scenario, ObjectiveKind::HandleAllProduced, resourceId) != nullptr;
}

ScenarioCatalog CreateTier0ScenarioCatalog()
{
    return ScenarioCatalog();
}
} // namespace graph
