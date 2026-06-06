#pragma once

#include "graph/ProductionTypes.h"

#include <string_view>
#include <vector>

namespace graph
{
class ResourceCatalog
{
public:
    ResourceCatalog();

    const std::vector<ResourceDef>& All() const;

    const ResourceDef* Find(ResourceId id) const;
    const ResourceDef* FindByKey(std::string_view key) const;

private:
    std::vector<ResourceDef> m_resources;
};

ResourceCatalog CreateTier0ResourceCatalog();
}
