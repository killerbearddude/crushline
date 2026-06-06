// Implements the hardcoded Tier 0 recipe catalog used by the production puzzle
// model. Recipes are the source of generated node ports and the input/output
// rates that upcoming evaluator patches will resolve.

#include "graph/RecipeCatalog.h"

namespace graph
{
namespace
{
std::vector<RecipeDef> BuildTier0Recipes()
{
    // NOTE: Tier 0 data is hardcoded while the gameplay model is being proven.
    // Later patches can move these definitions into external data files once the
    // schema stabilizes.
    return {
        {
            production_ids::ExtractIronOre,
            "extract_iron_ore",
            "Extract Iron Ore",
            MachineClass::Source,
            TechTier::Tier0,
            {},
            {{production_ids::IronOre, 60.0f}},
            {},
            1.0f,
            0.0f
        },
        {
            production_ids::SupplyWater,
            "supply_water",
            "Supply Water",
            MachineClass::Source,
            TechTier::Tier0,
            {},
            {{production_ids::Water, 60.0f}},
            {},
            1.0f,
            0.0f
        },
        {
            production_ids::CrushIronOre,
            "crush_iron_ore",
            "Crush Iron Ore",
            MachineClass::Crushing,
            TechTier::Tier0,
            {{production_ids::IronOre, 60.0f}},
            {{production_ids::CrushedIronOre, 60.0f}},
            {},
            1.0f,
            60.0f
        },
        {
            production_ids::WashCrushedIronOre,
            "wash_crushed_iron_ore",
            "Wash Crushed Iron Ore",
            MachineClass::Washing,
            TechTier::Tier0,
            {
                {production_ids::CrushedIronOre, 60.0f},
                {production_ids::Water, 30.0f}
            },
            {{production_ids::WashedIronOre, 50.0f}},
            {{production_ids::IronSlurry, 10.0f}},
            1.0f,
            90.0f
        },
        {
            production_ids::SmeltWashedIronOre,
            "smelt_washed_iron_ore",
            "Smelt Washed Iron Ore",
            MachineClass::Smelting,
            TechTier::Tier0,
            {{production_ids::WashedIronOre, 50.0f}},
            {{production_ids::IronIngot, 50.0f}},
            {},
            1.0f,
            120.0f
        },
        {
            production_ids::StoreIronSlurry,
            "store_iron_slurry",
            "Store Iron Slurry",
            MachineClass::WasteHandling,
            TechTier::Tier0,
            {{production_ids::IronSlurry, 10.0f}},
            {{production_ids::Waste, 10.0f}},
            {},
            1.0f,
            10.0f
        }
    };
}

bool ContainsResource(const std::vector<ResourceAmount>& amounts, ResourceId resourceId)
{
    for (const ResourceAmount& amount : amounts)
    {
        if (amount.resourceId == resourceId)
        {
            return true;
        }
    }

    return false;
}
} // namespace

RecipeCatalog::RecipeCatalog()
    : m_recipes(BuildTier0Recipes())
{
}

const std::vector<RecipeDef>& RecipeCatalog::All() const
{
    return m_recipes;
}

const RecipeDef* RecipeCatalog::Find(RecipeId id) const
{
    for (const RecipeDef& recipe : m_recipes)
    {
        if (recipe.id == id)
        {
            return &recipe;
        }
    }

    return nullptr;
}

const RecipeDef* RecipeCatalog::FindByKey(std::string_view key) const
{
    for (const RecipeDef& recipe : m_recipes)
    {
        if (recipe.key == key)
        {
            return &recipe;
        }
    }

    return nullptr;
}

std::vector<const RecipeDef*> RecipeCatalog::FindCompatibleRecipes(MachineClass machineClass) const
{
    std::vector<const RecipeDef*> recipes;

    for (const RecipeDef& recipe : m_recipes)
    {
        if (recipe.requiredMachineClass == machineClass)
        {
            recipes.push_back(&recipe);
        }
    }

    return recipes;
}

bool RecipeProducesResource(const RecipeDef& recipe, ResourceId resourceId)
{
    return ContainsResource(recipe.outputs, resourceId);
}

bool RecipeConsumesResource(const RecipeDef& recipe, ResourceId resourceId)
{
    return ContainsResource(recipe.inputs, resourceId);
}

bool RecipeCreatesByproduct(const RecipeDef& recipe, ResourceId resourceId)
{
    return ContainsResource(recipe.byproducts, resourceId);
}

RecipeCatalog CreateTier0RecipeCatalog()
{
    return RecipeCatalog();
}
} // namespace graph
