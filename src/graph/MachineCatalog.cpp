#include "graph/MachineCatalog.h"

namespace graph
{
namespace
{
std::vector<MachineDef> BuildTier0Machines()
{
    return {
        {
            production_ids::ResourceSource,
            "resource_source",
            "Resource Source",
            MachineClass::Source,
            TechTier::Tier0,
            0.0f,
            60.0f
        },
        {
            production_ids::Crusher,
            "crusher",
            "Crusher",
            MachineClass::Crushing,
            TechTier::Tier0,
            60.0f,
            60.0f
        },
        {
            production_ids::Washer,
            "washer",
            "Washer",
            MachineClass::Washing,
            TechTier::Tier0,
            90.0f,
            60.0f
        },
        {
            production_ids::Smelter,
            "smelter",
            "Smelter",
            MachineClass::Smelting,
            TechTier::Tier0,
            120.0f,
            50.0f
        },
        {
            production_ids::WasteSink,
            "waste_sink",
            "Waste Sink",
            MachineClass::WasteHandling,
            TechTier::Tier0,
            10.0f,
            60.0f
        }
    };
}
}

MachineCatalog::MachineCatalog()
    : m_machines(BuildTier0Machines())
{
}

const std::vector<MachineDef>& MachineCatalog::All() const
{
    return m_machines;
}

const MachineDef* MachineCatalog::Find(MachineId id) const
{
    for (const MachineDef& machine : m_machines)
    {
        if (machine.id == id)
        {
            return &machine;
        }
    }

    return nullptr;
}

const MachineDef* MachineCatalog::FindByKey(std::string_view key) const
{
    for (const MachineDef& machine : m_machines)
    {
        if (machine.key == key)
        {
            return &machine;
        }
    }

    return nullptr;
}

const MachineDef* MachineCatalog::FindByClass(MachineClass machineClass) const
{
    for (const MachineDef& machine : m_machines)
    {
        if (machine.machineClass == machineClass)
        {
            return &machine;
        }
    }

    return nullptr;
}

MachineCatalog CreateTier0MachineCatalog()
{
    return MachineCatalog();
}
}
