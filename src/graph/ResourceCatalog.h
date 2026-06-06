#pragma once

// Declares the resource catalog used by production recipes, graph ports, and
// future scenario objectives. The catalog owns stable Tier 0 resource definitions
// and exposes read-only lookup helpers for gameplay systems.

#include "graph/ProductionTypes.h"

#include <string_view>
#include <vector>

namespace graph
{
class ResourceCatalog
{
public:
    // Builds the current hardcoded Tier 0 resource set.
    ResourceCatalog();

    // Returns every resource definition owned by this catalog. The returned
    // reference remains valid for the lifetime of the catalog instance.
    [[nodiscard]] const std::vector<ResourceDef>& All() const;

    // Finds a resource by stable catalog ID. The returned pointer is non-owning
    // and remains valid until this catalog is destroyed; returns nullptr when the
    // ID is unknown.
    [[nodiscard]] const ResourceDef* Find(ResourceId id) const;

    // Finds a resource by content key, such as "iron_ore". The returned pointer
    // is non-owning and remains valid until this catalog is destroyed; returns
    // nullptr when the key is unknown.
    [[nodiscard]] const ResourceDef* FindByKey(std::string_view key) const;

private:
    std::vector<ResourceDef> m_resources;
};

// Creates the current Tier 0 resource catalog. This free function keeps call
// sites explicit when future patches introduce tier-specific or data-file-backed
// catalog construction.
[[nodiscard]] ResourceCatalog CreateTier0ResourceCatalog();
} // namespace graph
