#pragma once

#include "graph/ProductionTypes.h"

#include <string_view>
#include <vector>

namespace graph
{
class MachineCatalog
{
public:
    MachineCatalog();

    const std::vector<MachineDef>& All() const;

    const MachineDef* Find(MachineId id) const;
    const MachineDef* FindByKey(std::string_view key) const;
    const MachineDef* FindByClass(MachineClass machineClass) const;

private:
    std::vector<MachineDef> m_machines;
};

MachineCatalog CreateTier0MachineCatalog();
}
