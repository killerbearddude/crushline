#pragma once

// Defines strongly typed production identifiers, resource categories, machine
// classes, and recipe data structures used by the production catalogs.
// These types are gameplay data only; graph rendering and UI state may reference
// them, but catalog content is owned by the catalog classes.

#include <string>
#include <vector>

namespace graph
{
// Lightweight domain IDs used by production catalogs and, later, recipe-driven
// graph nodes. Zero is reserved as the invalid value so default construction is
// easy to detect during validation and tests.
using ResourceId = int;
using MachineId = int;
using RecipeId = int;

inline constexpr ResourceId InvalidResourceId = 0;
inline constexpr MachineId InvalidMachineId = 0;
inline constexpr RecipeId InvalidRecipeId = 0;

// Unlock tier for production content. Early patches use Tier0 only; higher tiers
// are reserved for the future tech-tree progression described in the design doc.
enum class TechTier
{
    Tier0 = 0,
    Tier1,
    Tier2,
    Tier3,
    Tier4,
    Tier5
};

// Broad resource category used for display, filtering, and later validation of
// wildcard ports. Exact resource identity remains the current connection rule.
enum class ResourceKind
{
    Solid,
    Fluid,
    Gas,
    Energy,
    Waste,
    Abstract
};

// Machine class describes the operation family a recipe requires. A machine can
// run recipes whose required class matches its own class.
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

// Static definition for a resource that can flow through graph edges. Boolean
// tags are gameplay hints for objectives, UI filtering, and waste/byproduct rules.
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

// Static definition for a machine type the player can eventually place. Machines
// own capability and baseline cost; recipes define the actual transformations.
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

// Resource rate used by recipe inputs, outputs, and byproducts. Rates are stored
// per minute because scenario targets are also expressed as production rates.
struct ResourceAmount
{
    ResourceId resourceId = InvalidResourceId;
    float ratePerMinute = 0.0f;
};

// Static definition for a recipe. Recipes are the source of generated graph ports:
// inputs become input ports, while outputs and byproducts become output ports.
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
// Stable Tier 0 IDs. These remain fixed so tests, sample graphs, serializers,
// and future scenario definitions can refer to catalog content deterministically.
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
