#pragma once

#include <string>
#include <vector>

namespace graph
{
using ResourceId = int;
using MachineId = int;
using RecipeId = int;

inline constexpr ResourceId InvalidResourceId = 0;
inline constexpr MachineId InvalidMachineId = 0;
inline constexpr RecipeId InvalidRecipeId = 0;

enum class TechTier
{
    Tier0 = 0,
    Tier1,
    Tier2,
    Tier3,
    Tier4,
    Tier5
};

enum class ResourceKind
{
    Solid,
    Fluid,
    Gas,
    Energy,
    Waste,
    Abstract
};

enum class MachineClass
{
    Source,
    Crushing,
    Washing,
    Smelting,
    Mixing,
    Separating,
    Chemical,
    Refining,
    Storage,
    WasteHandling
};

struct ResourceDef
{
    ResourceId id = InvalidResourceId;
    std::string key;
    std::string displayName;
    ResourceKind kind = ResourceKind::Solid;
    TechTier tier = TechTier::Tier0;
    bool isWaste = false;
    bool isRaw = false;
    bool isFinal = false;
};

struct MachineDef
{
    MachineId id = InvalidMachineId;
    std::string key;
    std::string displayName;
    MachineClass machineClass = MachineClass::Source;
    TechTier tier = TechTier::Tier0;
    float basePowerKw = 0.0f;
    float baseThroughputPerMinute = 0.0f;
};

struct ResourceAmount
{
    ResourceId resourceId = InvalidResourceId;
    float ratePerMinute = 0.0f;
};

struct RecipeDef
{
    RecipeId id = InvalidRecipeId;
    std::string key;
    std::string displayName;

    MachineClass requiredMachineClass = MachineClass::Source;
    TechTier tier = TechTier::Tier0;

    std::vector<ResourceAmount> inputs;
    std::vector<ResourceAmount> outputs;
    std::vector<ResourceAmount> byproducts;

    float durationSeconds = 1.0f;
    float powerKw = 0.0f;
};

namespace production_ids
{
inline constexpr ResourceId IronOre = 1;
inline constexpr ResourceId CrushedIronOre = 2;
inline constexpr ResourceId WashedIronOre = 3;
inline constexpr ResourceId IronSlurry = 4;
inline constexpr ResourceId IronIngot = 5;
inline constexpr ResourceId Water = 6;
inline constexpr ResourceId Waste = 7;

inline constexpr MachineId ResourceSource = 1;
inline constexpr MachineId Crusher = 2;
inline constexpr MachineId Washer = 3;
inline constexpr MachineId Smelter = 4;
inline constexpr MachineId WasteSink = 5;

inline constexpr RecipeId ExtractIronOre = 1;
inline constexpr RecipeId SupplyWater = 2;
inline constexpr RecipeId CrushIronOre = 3;
inline constexpr RecipeId WashCrushedIronOre = 4;
inline constexpr RecipeId SmeltWashedIronOre = 5;
inline constexpr RecipeId StoreIronSlurry = 6;
} // namespace production_ids

} // namespace graph
