#pragma once

// Declares the machine catalog used by recipe compatibility checks and future
// node creation. Machines describe capability classes and baseline costs; recipes
// describe the actual resource transformations.

#include "graph/ProductionTypes.h"

#include <string_view>
#include <vector>

namespace graph
{
class MachineCatalog
{
public:
    // Builds the current hardcoded Tier 0 machine set.
    MachineCatalog();

    // Returns every machine definition owned by this catalog. The returned
    // reference remains valid for the lifetime of the catalog instance.
    [[nodiscard]] const std::vector<MachineDef>& All() const;

    // Finds a machine by stable catalog ID. The returned pointer is non-owning
    // and remains valid until this catalog is destroyed; returns nullptr when the
    // ID is unknown.
    [[nodiscard]] const MachineDef* Find(MachineId id) const;

    // Finds a machine by content key, such as "crusher". The returned pointer is
    // non-owning and remains valid until this catalog is destroyed; returns
    // nullptr when the key is unknown.
    [[nodiscard]] const MachineDef* FindByKey(std::string_view key) const;

    // Finds the first machine that supports the requested class. This is useful
    // for tests and early automatic node creation; future UI code may expose all
    // machines in a class once multiple machines per class exist.
    [[nodiscard]] const MachineDef* FindByClass(MachineClass machineClass) const;

private:
    std::vector<MachineDef> m_machines;
};

// Creates the current Tier 0 machine catalog. This mirrors the other catalog
// factories and leaves room for later data-file-backed construction.
[[nodiscard]] MachineCatalog CreateTier0MachineCatalog();
} // namespace graph
