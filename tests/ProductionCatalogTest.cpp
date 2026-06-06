#include "graph/MachineCatalog.h"
#include "graph/RecipeCatalog.h"
#include "graph/ResourceCatalog.h"

#include <cmath>
#include <iostream>
#include <string>

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

bool CheckTier0Resources()
{
    const graph::ResourceCatalog resources;

    const graph::ResourceDef* ironOre = resources.FindByKey("iron_ore");
    const graph::ResourceDef* water = resources.FindByKey("water");
    const graph::ResourceDef* slurry = resources.FindByKey("iron_slurry");
    const graph::ResourceDef* ingot = resources.FindByKey("iron_ingot");

    return
        Check(resources.All().size() == 7, "Tier 0 should expose seven resources") &&
        Check(ironOre != nullptr, "Iron Ore resource should be findable by key") &&
        Check(ironOre->id == graph::production_ids::IronOre, "Iron Ore id mismatch") &&
        Check(ironOre->kind == graph::ResourceKind::Solid, "Iron Ore should be a solid") &&
        Check(ironOre->isRaw, "Iron Ore should be marked raw") &&
        Check(!ironOre->isWaste, "Iron Ore should not be waste") &&
        Check(water != nullptr, "Water resource should be findable by key") &&
        Check(water->kind == graph::ResourceKind::Fluid, "Water should be a fluid") &&
        Check(water->isRaw, "Water should be marked raw") &&
        Check(slurry != nullptr, "Iron Slurry resource should be findable by key") &&
        Check(slurry->kind == graph::ResourceKind::Waste, "Iron Slurry should use waste kind") &&
        Check(slurry->isWaste, "Iron Slurry should be marked waste") &&
        Check(ingot != nullptr, "Iron Ingot resource should be findable by key") &&
        Check(ingot->isFinal, "Iron Ingot should be marked final") &&
        Check(resources.Find(graph::production_ids::WashedIronOre) != nullptr, "Washed Iron Ore id lookup should work") &&
        Check(resources.Find(99999) == nullptr, "Unknown resource lookup should return null");
}

bool CheckTier0Machines()
{
    const graph::MachineCatalog machines;

    const graph::MachineDef* crusher = machines.FindByKey("crusher");
    const graph::MachineDef* washer = machines.FindByKey("washer");
    const graph::MachineDef* smelter = machines.FindByKey("smelter");

    return
        Check(machines.All().size() == 5, "Tier 0 should expose five machines") &&
        Check(crusher != nullptr, "Crusher should be findable by key") &&
        Check(crusher->id == graph::production_ids::Crusher, "Crusher id mismatch") &&
        Check(crusher->machineClass == graph::MachineClass::Crushing, "Crusher class mismatch") &&
        CheckNear(crusher->basePowerKw, 60.0f, "Crusher base power mismatch") &&
        Check(washer != nullptr, "Washer should be findable by key") &&
        Check(washer->machineClass == graph::MachineClass::Washing, "Washer class mismatch") &&
        CheckNear(washer->basePowerKw, 90.0f, "Washer base power mismatch") &&
        Check(smelter != nullptr, "Smelter should be findable by key") &&
        Check(smelter->machineClass == graph::MachineClass::Smelting, "Smelter class mismatch") &&
        Check(machines.FindByClass(graph::MachineClass::Source) != nullptr, "Source class lookup should find Resource Source") &&
        Check(machines.FindByClass(graph::MachineClass::Chemical) == nullptr, "Locked Chemical class should not be present in Tier 0") &&
        Check(machines.Find(99999) == nullptr, "Unknown machine lookup should return null");
}

bool CheckTier0Recipes()
{
    const graph::RecipeCatalog recipes;

    const graph::RecipeDef* crush = recipes.FindByKey("crush_iron_ore");
    const graph::RecipeDef* wash = recipes.FindByKey("wash_crushed_iron_ore");
    const graph::RecipeDef* smelt = recipes.FindByKey("smelt_washed_iron_ore");
    const graph::RecipeDef* store = recipes.FindByKey("store_iron_slurry");

    if (!Check(recipes.All().size() == 6, "Tier 0 should expose six recipes") ||
        !Check(crush != nullptr, "Crush Iron Ore recipe should exist") ||
        !Check(wash != nullptr, "Wash Crushed Iron Ore recipe should exist") ||
        !Check(smelt != nullptr, "Smelt Washed Iron Ore recipe should exist") ||
        !Check(store != nullptr, "Store Iron Slurry recipe should exist"))
    {
        return false;
    }

    const graph::ResourceAmount* crushInput = FindAmount(crush->inputs, graph::production_ids::IronOre);
    const graph::ResourceAmount* crushOutput = FindAmount(crush->outputs, graph::production_ids::CrushedIronOre);

    const graph::ResourceAmount* washOreInput = FindAmount(wash->inputs, graph::production_ids::CrushedIronOre);
    const graph::ResourceAmount* washWaterInput = FindAmount(wash->inputs, graph::production_ids::Water);
    const graph::ResourceAmount* washOutput = FindAmount(wash->outputs, graph::production_ids::WashedIronOre);
    const graph::ResourceAmount* washByproduct = FindAmount(wash->byproducts, graph::production_ids::IronSlurry);

    const graph::ResourceAmount* smeltInput = FindAmount(smelt->inputs, graph::production_ids::WashedIronOre);
    const graph::ResourceAmount* smeltOutput = FindAmount(smelt->outputs, graph::production_ids::IronIngot);

    const graph::ResourceAmount* storeInput = FindAmount(store->inputs, graph::production_ids::IronSlurry);
    const graph::ResourceAmount* storeOutput = FindAmount(store->outputs, graph::production_ids::Waste);

    return
        Check(crush->requiredMachineClass == graph::MachineClass::Crushing, "Crush recipe should require crushing") &&
        Check(crushInput != nullptr, "Crush recipe should consume Iron Ore") &&
        CheckNear(crushInput->ratePerMinute, 60.0f, "Crush Iron Ore input rate mismatch") &&
        Check(crushOutput != nullptr, "Crush recipe should produce Crushed Iron Ore") &&
        CheckNear(crushOutput->ratePerMinute, 60.0f, "Crush Iron Ore output rate mismatch") &&
        Check(wash->requiredMachineClass == graph::MachineClass::Washing, "Wash recipe should require washing") &&
        Check(washOreInput != nullptr, "Wash recipe should consume Crushed Iron Ore") &&
        CheckNear(washOreInput->ratePerMinute, 60.0f, "Wash ore input rate mismatch") &&
        Check(washWaterInput != nullptr, "Wash recipe should consume Water") &&
        CheckNear(washWaterInput->ratePerMinute, 30.0f, "Wash water input rate mismatch") &&
        Check(washOutput != nullptr, "Wash recipe should produce Washed Iron Ore") &&
        CheckNear(washOutput->ratePerMinute, 50.0f, "Wash output rate mismatch") &&
        Check(washByproduct != nullptr, "Wash recipe should create Iron Slurry byproduct") &&
        CheckNear(washByproduct->ratePerMinute, 10.0f, "Wash byproduct rate mismatch") &&
        Check(smeltInput != nullptr, "Smelt recipe should consume Washed Iron Ore") &&
        Check(smeltOutput != nullptr, "Smelt recipe should produce Iron Ingot") &&
        CheckNear(smeltOutput->ratePerMinute, 50.0f, "Smelt output rate mismatch") &&
        Check(storeInput != nullptr, "Store recipe should consume Iron Slurry") &&
        Check(storeOutput != nullptr, "Store recipe should output Waste") &&
        Check(graph::RecipeConsumesResource(*wash, graph::production_ids::Water), "RecipeConsumesResource should detect water input") &&
        Check(graph::RecipeProducesResource(*smelt, graph::production_ids::IronIngot), "RecipeProducesResource should detect ingot output") &&
        Check(graph::RecipeCreatesByproduct(*wash, graph::production_ids::IronSlurry), "RecipeCreatesByproduct should detect slurry byproduct") &&
        Check(!graph::RecipeProducesResource(*crush, graph::production_ids::IronIngot), "Crush should not produce ingots");
}

bool CheckRecipeMachineCompatibility()
{
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
}

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
