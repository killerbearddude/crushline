#pragma once

#include "graph/ProductionTypes.h"

#include <string_view>
#include <vector>

namespace graph
{
class RecipeCatalog
{
public:
    RecipeCatalog();

    const std::vector<RecipeDef>& All() const;

    const RecipeDef* Find(RecipeId id) const;
    const RecipeDef* FindByKey(std::string_view key) const;
    std::vector<const RecipeDef*> FindCompatibleRecipes(MachineClass machineClass) const;

private:
    std::vector<RecipeDef> m_recipes;
};

bool RecipeProducesResource(const RecipeDef& recipe, ResourceId resourceId);
bool RecipeConsumesResource(const RecipeDef& recipe, ResourceId resourceId);
bool RecipeCreatesByproduct(const RecipeDef& recipe, ResourceId resourceId);

RecipeCatalog CreateTier0RecipeCatalog();
}
