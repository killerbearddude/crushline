#include "graph/ResourceCatalog.h"

namespace graph
{
namespace
{
std::vector<ResourceDef> BuildTier0Resources()
{
    return {
        {
            production_ids::IronOre,
            "iron_ore",
            "Iron Ore",
            ResourceKind::Solid,
            TechTier::Tier0,
            false,
            true,
            false
        },
        {
            production_ids::CrushedIronOre,
            "crushed_iron_ore",
            "Crushed Iron Ore",
            ResourceKind::Solid,
            TechTier::Tier0,
            false,
            false,
            false
        },
        {
            production_ids::WashedIronOre,
            "washed_iron_ore",
            "Washed Iron Ore",
            ResourceKind::Solid,
            TechTier::Tier0,
            false,
            false,
            false
        },
        {
            production_ids::IronSlurry,
            "iron_slurry",
            "Iron Slurry",
            ResourceKind::Waste,
            TechTier::Tier0,
            true,
            false,
            false
        },
        {
            production_ids::IronIngot,
            "iron_ingot",
            "Iron Ingot",
            ResourceKind::Solid,
            TechTier::Tier0,
            false,
            false,
            true
        },
        {
            production_ids::Water,
            "water",
            "Water",
            ResourceKind::Fluid,
            TechTier::Tier0,
            false,
            true,
            false
        },
        {
            production_ids::Waste,
            "waste",
            "Waste",
            ResourceKind::Waste,
            TechTier::Tier0,
            true,
            false,
            false
        }
    };
}
}

ResourceCatalog::ResourceCatalog()
    : m_resources(BuildTier0Resources())
{
}

const std::vector<ResourceDef>& ResourceCatalog::All() const
{
    return m_resources;
}

const ResourceDef* ResourceCatalog::Find(ResourceId id) const
{
    for (const ResourceDef& resource : m_resources)
    {
        if (resource.id == id)
        {
            return &resource;
        }
    }

    return nullptr;
}

const ResourceDef* ResourceCatalog::FindByKey(std::string_view key) const
{
    for (const ResourceDef& resource : m_resources)
    {
        if (resource.key == key)
        {
            return &resource;
        }
    }

    return nullptr;
}

ResourceCatalog CreateTier0ResourceCatalog()
{
    return ResourceCatalog();
}
}
