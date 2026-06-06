// Verifies the Tier 0 production catalogs that seed the first real gameplay
// loop. These tests protect resource IDs, recipe rates, byproduct semantics, and
// machine compatibility before graph nodes begin generating ports from recipes.

#include "graph/MachineCatalog.h"
#include "graph/RecipeCatalog.h"
#include "graph/ResourceCatalog.h"

#include <cmath>
#include <iostream>
#include <string_view>
#include <vector>

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
        std::cerr << "Production catalog test failed: " << message << "\n";
        return false;
    }

    return true;
}

bool CheckNear(float actual, float expected, const char* message)
{
    if (!NearlyEqual(actual, expected))
    {
        std::cerr
            << "Production catalog test failed: " << message
            << " expected=" << expected
            << " actual=" << actual
            << "\n";
        return false;
    }

    return true;
}

const graph::ResourceDef* RequireResourceByKey(
    const graph::ResourceCatalog& catalog,
    std::string_view key
)
{
    const graph::ResourceDef* resource = catalog.FindByKey(key);
    Check(resource != nullptr, "Required resource key should exist");
    return resource;
}

const graph::MachineDef* RequireMachineByKey(
    const graph::MachineCatalog& catalog,
    std::string_view key
)
{
    const graph::MachineDef* machine = catalog.FindByKey(key);
    Check(machine != nullptr, "Required machine key should exist");
    return machine;
}

const graph::RecipeDef* RequireRecipeByKey(
    const graph::RecipeCatalog& catalog,
    std::string_view key
)
{
    const graph::RecipeDef* recipe = catalog.FindByKey(key);
    Check(recipe != nullptr, "Required recipe key should exist");
    return recipe;
}

const graph::ResourceAmount* FindAmount(
    const std::vector<graph::ResourceAmount>& amounts,
    graph::ResourceId resourceId
)
{
    for (const graph::ResourceAmount& amount : amounts)
    {
        if (amount.resourceId == resourceId)
        {
            return &amount;
        }
    }

    return nullptr;
}

bool CheckAmount(
    const std::vector<graph::ResourceAmount>& amounts,
    graph::ResourceId resourceId,
    float expectedRate,
    const char* missingMessage,
    const char* rateMessage
)
{
    const graph::ResourceAmount* amount = FindAmount(amounts, resourceId);
    return
        Check(amount != nullptr, missingMessage) &&
        CheckNear(amount->ratePerMinute, expectedRate, rateMessage);
}

bool CheckTier0Resources()
{
    // Verifies the starter resource set and its semantic tags. This prevents
    // regressions where future recipe-node generation would create ports from
    // missing or incorrectly classified resources.
    const graph::ResourceCatalog resources;

    const graph::ResourceDef* ironOre = RequireResourceByKey(resources, "iron_ore");
    const graph::ResourceDef* water = RequireResourceByKey(resources, "water");
    const graph::ResourceDef* slurry = RequireResourceByKey(resources, "iron_slurry");
    const graph::ResourceDef* ingot = RequireResourceByKey(resources, "iron_ingot");

    return
        Check(resources.All().size() == 7, "Tier 0 should expose seven resources") &&
        Check(ironOre != nullptr, "Iron Ore resource should be required by key") &&
        Check(ironOre->id == graph::production_ids::IronOre, "Iron Ore id mismatch") &&
        Check(ironOre->kind == graph::ResourceKind::Solid, "Iron Ore should be a solid") &&
        Check(ironOre->isRaw, "Iron Ore should be marked raw") &&
        Check(!ironOre->isWaste, "Iron Ore should not be waste") &&
        Check(water != nullptr, "Water resource should be required by key") &&
        Check(water->kind == graph::ResourceKind::Fluid, "Water should be a fluid") &&
        Check(water->isRaw, "Water should be marked raw") &&
        Check(slurry != nullptr, "Iron Slurry resource should be required by key") &&
        Check(slurry->kind == graph::ResourceKind::Waste, "Iron Slurry should use waste kind") &&
        Check(slurry->isWaste, "Iron Slurry should be marked waste") &&
        Check(ingot != nullptr, "Iron Ingot resource should be required by key") &&
        Check(ingot->isFinal, "Iron Ingot should be marked final") &&
        Check(resources.Find(graph::production_ids::WashedIronOre) != nullptr, "Washed Iron Ore id lookup should work") &&
        Check(resources.Find(99999) == nullptr, "Unknown resource lookup should return null");
}

bool CheckTier0Machines()
{
    // Verifies the initial machine classes and baseline costs. Recipe-driven
    // graph nodes will use these classes to reject incompatible recipes.
    const graph::MachineCatalog machines;

    const graph::MachineDef* crusher = RequireMachineByKey(machines, "crusher");
    const graph::MachineDef* washer = RequireMachineByKey(machines, "washer");
    const graph::MachineDef* smelter = RequireMachineByKey(machines, "smelter");

    return
        Check(machines.All().size() == 5, "Tier 0 should expose five machines") &&
        Check(crusher != nullptr, "Crusher should be required by key") &&
        Check(crusher->id == graph::production_ids::Crusher, "Crusher id mismatch") &&
        Check(crusher->machineClass == graph::MachineClass::Crushing, "Crusher class mismatch") &&
        CheckNear(crusher->basePowerKw, 60.0f, "Crusher base power mismatch") &&
        Check(washer != nullptr, "Washer should be required by key") &&
        Check(washer->machineClass == graph::MachineClass::Washing, "Washer class mismatch") &&
        CheckNear(washer->basePowerKw, 90.0f, "Washer base power mismatch") &&
        Check(smelter != nullptr, "Smelter should be required by key") &&
        Check(smelter->machineClass == graph::MachineClass::Smelting, "Smelter class mismatch") &&
        Check(machines.FindByClass(graph::MachineClass::Source) != nullptr, "Source class lookup should find Resource Source") &&
        Check(machines.FindByClass(graph::MachineClass::Chemical) == nullptr, "Locked Chemical class should not be present in Tier 0") &&
        Check(machines.Find(99999) == nullptr, "Unknown machine lookup should return null");
}

bool CheckTier0Recipes()
{
    // Verifies recipe inputs, outputs, byproducts, and rates. This prevents
    // regressions where the puzzle chain could no longer produce the Tier 0
    // objective or route its slurry byproduct.
    const graph::RecipeCatalog recipes;

    const graph::RecipeDef* crush = RequireRecipeByKey(recipes, "crush_iron_ore");
    const graph::RecipeDef* wash = RequireRecipeByKey(recipes, "wash_crushed_iron_ore");
    const graph::RecipeDef* smelt = RequireRecipeByKey(recipes, "smelt_washed_iron_ore");
    const graph::RecipeDef* store = RequireRecipeByKey(recipes, "store_iron_slurry");

    if (!Check(recipes.All().size() == 6, "Tier 0 should expose six recipes") ||
        !Check(crush != nullptr, "Crush Iron Ore recipe should exist") ||
        !Check(wash != nullptr, "Wash Crushed Iron Ore recipe should exist") ||
        !Check(smelt != nullptr, "Smelt Washed Iron Ore recipe should exist") ||
        !Check(store != nullptr, "Store Iron Slurry recipe should exist"))
    {
        return false;
    }

    return
        Check(crush->requiredMachineClass == graph::MachineClass::Crushing, "Crush recipe should require crushing") &&
        CheckAmount(crush->inputs, graph::production_ids::IronOre, 60.0f, "Crush recipe should consume Iron Ore", "Crush Iron Ore input rate mismatch") &&
        CheckAmount(crush->outputs, graph::production_ids::CrushedIronOre, 60.0f, "Crush recipe should produce Crushed Iron Ore", "Crush Iron Ore output rate mismatch") &&
        Check(wash->requiredMachineClass == graph::MachineClass::Washing, "Wash recipe should require washing") &&
        CheckAmount(wash->inputs, graph::production_ids::CrushedIronOre, 60.0f, "Wash recipe should consume Crushed Iron Ore", "Wash ore input rate mismatch") &&
        CheckAmount(wash->inputs, graph::production_ids::Water, 30.0f, "Wash recipe should consume Water", "Wash water input rate mismatch") &&
        CheckAmount(wash->outputs, graph::production_ids::WashedIronOre, 50.0f, "Wash recipe should produce Washed Iron Ore", "Wash output rate mismatch") &&
        CheckAmount(wash->byproducts, graph::production_ids::IronSlurry, 10.0f, "Wash recipe should create Iron Slurry byproduct", "Wash byproduct rate mismatch") &&
        CheckAmount(smelt->inputs, graph::production_ids::WashedIronOre, 50.0f, "Smelt recipe should consume Washed Iron Ore", "Smelt input rate mismatch") &&
        CheckAmount(smelt->outputs, graph::production_ids::IronIngot, 50.0f, "Smelt recipe should produce Iron Ingot", "Smelt output rate mismatch") &&
        CheckAmount(store->inputs, graph::production_ids::IronSlurry, 10.0f, "Store recipe should consume Iron Slurry", "Store input rate mismatch") &&
        CheckAmount(store->outputs, graph::production_ids::Waste, 10.0f, "Store recipe should output Waste", "Store output rate mismatch") &&
        Check(graph::RecipeConsumesResource(*wash, graph::production_ids::Water), "RecipeConsumesResource should detect water input") &&
        Check(graph::RecipeProducesResource(*smelt, graph::production_ids::IronIngot), "RecipeProducesResource should detect ingot output") &&
        Check(graph::RecipeCreatesByproduct(*wash, graph::production_ids::IronSlurry), "RecipeCreatesByproduct should detect slurry byproduct") &&
        Check(!graph::RecipeProducesResource(*crush, graph::production_ids::IronIngot), "Crush should not produce ingots");
}

bool CheckRecipeMachineCompatibility()
{
    // Verifies that compatible-recipe lookup stays class-based rather than key-
    // or name-based. This protects future machine nodes from accepting recipes
    // that do not belong to their machine class.
    const graph::RecipeCatalog recipes;

    const std::vector<const graph::RecipeDef*> sourceRecipes = recipes.FindCompatibleRecipes(graph::MachineClass::Source);
    const std::vector<const graph::RecipeDef*> washingRecipes = recipes.FindCompatibleRecipes(graph::MachineClass::Washing);
    const std::vector<const graph::RecipeDef*> chemicalRecipes = recipes.FindCompatibleRecipes(graph::MachineClass::Chemical);

    return
        Check(sourceRecipes.size() == 2, "Source machine class should have two Tier 0 recipes") &&
        Check(washingRecipes.size() == 1, "Washing machine class should have one Tier 0 recipe") &&
        Check(washingRecipes.front()->id == graph::production_ids::WashCrushedIronOre, "Washing recipe id mismatch") &&
        Check(chemicalRecipes.empty(), "Chemical machine class should have no Tier 0 recipes");
}

bool CheckCatalogIntegrity()
{
    // Verifies cross-catalog integrity so every recipe references real resources
    // with positive rates. This catches data-entry mistakes before they become
    // graph evaluation or generated-port bugs.
    const graph::ResourceCatalog resources;
    const graph::RecipeCatalog recipes;

    for (const graph::RecipeDef& recipe : recipes.All())
    {
        if (!Check(recipe.id != graph::InvalidRecipeId, "Recipe id should be valid") ||
            !Check(!recipe.key.empty(), "Recipe key should be non-empty") ||
            !Check(!recipe.displayName.empty(), "Recipe display name should be non-empty") ||
            !Check(recipe.durationSeconds > 0.0f, "Recipe duration should be positive"))
        {
            return false;
        }

        for (const graph::ResourceAmount& amount : recipe.inputs)
        {
            if (!Check(resources.Find(amount.resourceId) != nullptr, "Recipe input resource should exist") ||
                !Check(amount.ratePerMinute > 0.0f, "Recipe input rate should be positive"))
            {
                return false;
            }
        }

        for (const graph::ResourceAmount& amount : recipe.outputs)
        {
            if (!Check(resources.Find(amount.resourceId) != nullptr, "Recipe output resource should exist") ||
                !Check(amount.ratePerMinute > 0.0f, "Recipe output rate should be positive"))
            {
                return false;
            }
        }

        for (const graph::ResourceAmount& amount : recipe.byproducts)
        {
            if (!Check(resources.Find(amount.resourceId) != nullptr, "Recipe byproduct resource should exist") ||
                !Check(amount.ratePerMinute > 0.0f, "Recipe byproduct rate should be positive"))
            {
                return false;
            }
        }
    }

    return true;
}
} // namespace

int RunProductionCatalogTest()
{
    if (!CheckTier0Resources() ||
        !CheckTier0Machines() ||
        !CheckTier0Recipes() ||
        !CheckRecipeMachineCompatibility() ||
        !CheckCatalogIntegrity())
    {
        return 1;
    }

    return 0;
}
