#pragma once

// Declares the recipe catalog used by production-chain evaluation and future
// recipe-driven graph nodes. Recipes are the authoritative source for generated
// node ports, production rates, byproducts, and machine compatibility.

#include "graph/ProductionTypes.h"

#include <string_view>
#include <vector>

namespace graph
{
class RecipeCatalog
{
public:
    // Builds the current hardcoded Tier 0 recipe set.
    RecipeCatalog();

    // Returns every recipe definition owned by this catalog. The returned
    // reference remains valid for the lifetime of the catalog instance.
    [[nodiscard]] const std::vector<RecipeDef>& All() const;

    // Finds a recipe by stable catalog ID. The returned pointer is non-owning and
    // remains valid until this catalog is destroyed; returns nullptr when the ID
    // is unknown.
    [[nodiscard]] const RecipeDef* Find(RecipeId id) const;

    // Finds a recipe by content key, such as "crush_iron_ore". The returned
    // pointer is non-owning and remains valid until this catalog is destroyed;
    // returns nullptr when the key is unknown.
    [[nodiscard]] const RecipeDef* FindByKey(std::string_view key) const;

    // Returns non-owning pointers to recipes that can run on the requested
    // machine class. Pointers remain valid until this catalog is destroyed.
    [[nodiscard]] std::vector<const RecipeDef*> FindCompatibleRecipes(MachineClass machineClass) const;

private:
    std::vector<RecipeDef> m_recipes;
};

// Returns true when the recipe has a normal output for the resource. Byproducts
// are intentionally checked through RecipeCreatesByproduct so callers can treat
// waste-routing and objective production differently.
[[nodiscard]] bool RecipeProducesResource(const RecipeDef& recipe, ResourceId resourceId);

// Returns true when the recipe consumes the resource as an input.
[[nodiscard]] bool RecipeConsumesResource(const RecipeDef& recipe, ResourceId resourceId);

// Returns true when the recipe creates the resource as a byproduct.
[[nodiscard]] bool RecipeCreatesByproduct(const RecipeDef& recipe, ResourceId resourceId);

// Creates the current Tier 0 recipe catalog. This mirrors the other catalog
// factories and leaves room for later data-file-backed construction.
[[nodiscard]] RecipeCatalog CreateTier0RecipeCatalog();
} // namespace graph
