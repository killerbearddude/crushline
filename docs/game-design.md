# Crushline Game Design

Status: Working design document  
Recommended repository path: `docs/game-design.md`  
Last updated: 2026-06-06

---

## 1. North Star

Crushline is a node-graph production puzzle game.

Players unlock resources, machines, and recipes through a tech tree, then build graph-based production chains that transform raw inputs into required target goods. The challenge is discovering valid process paths, balancing ratios, routing byproducts, resolving bottlenecks, and satisfying scenario objectives efficiently.

The production graph is the factory.

Crushline is not primarily a physical-space factory builder. The player is not placing belts and machines on terrain. The main game board is an interactive node graph similar in interaction model to tools such as Blender node graphs, Dynamo, Unreal Blueprints, Substance Designer, Houdini networks, and other professional graph editors.

Reference process diagrams should be treated as examples of recipe dependency complexity, not as visual UI targets.

---

## 2. Core Game Concept

The player starts with simple raw materials and a limited set of unlocked processing methods. Each scenario gives one or more production goals, such as producing a target resource at a required rate. The player constructs a graph of machines and recipes to satisfy those goals.

A valid graph must obey recipe requirements, resource compatibility, machine capabilities, throughput constraints, and progression locks. A good graph satisfies the target with fewer bottlenecks, less waste, better power efficiency, simpler routing, or higher surplus value.

The game is a puzzle of production causality:

```text
Given this target item, what chain of resources, machines, recipes, and byproduct handling can produce it?
```

---

## 3. Core Player Loop

1. The player receives or selects a scenario objective.
2. The player reviews currently unlocked resources, machines, and recipes.
3. The player places machine or process nodes onto the graph canvas.
4. The player selects recipes for machines where applicable.
5. The player connects output ports to compatible input ports.
6. The evaluator resolves the graph and reports validity, target satisfaction, bottlenecks, deficits, surplus, waste, and byproducts.
7. The player edits the graph until the objective is satisfied.
8. Completion unlocks additional recipes, machine classes, resources, or later scenarios.

---

## 4. Primary Game Systems

### 4.1 Resource Catalog

Resources are the items, fluids, gases, waste products, and intermediates that flow through the graph.

Examples:

- Iron Ore
- Crushed Iron Ore
- Washed Iron Ore
- Iron Slurry
- Iron Ingot
- Water
- Oxygen
- Hydrogen
- Sulfur Dust
- Sulfuric Acid
- Waste

Resource definitions should eventually include:

```cpp
struct ResourceDef
{
    ResourceId id;
    std::string key;
    std::string displayName;
    ResourceKind kind;
    TechTier tier;
    bool isWaste = false;
    bool isRaw = false;
    bool isFinal = false;
};
```

Suggested `ResourceKind` values:

```cpp
enum class ResourceKind
{
    Solid,
    Fluid,
    Gas,
    Energy,
    Waste,
    Abstract
};
```

### 4.2 Machine Catalog

Machines define what kinds of processing actions are available. They do not by themselves define every transformation; recipes do that.

Examples:

- Resource Source
- Crusher
- Washer
- Smelter
- Chemical Infuser
- Electrolytic Separator
- Mixer
- Separator
- Refinery
- Waste Sink

Machine definitions should eventually include:

```cpp
struct MachineDef
{
    MachineId id;
    std::string key;
    std::string displayName;
    MachineClass machineClass;
    TechTier tier;
    float basePowerKw = 0.0f;
    float baseThroughputPerMinute = 0.0f;
};
```

Suggested `MachineClass` values:

```cpp
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
```

### 4.3 Recipe Catalog

Recipes are the heart of the game. A recipe declares required inputs, produced outputs, byproducts, waste products, compatible machine classes, processing time, and unlock tier.

Examples:

- Extract Iron Ore
- Crush Iron Ore
- Wash Crushed Iron Ore
- Smelt Washed Iron Ore
- Store Iron Slurry
- Electrolyze Water
- Mix Sulfur Trioxide and Water Vapor

Recipe definitions should eventually include:

```cpp
struct ResourceAmount
{
    ResourceId resourceId;
    float ratePerMinute = 0.0f;
};

struct RecipeDef
{
    RecipeId id;
    std::string key;
    std::string displayName;

    MachineClass requiredMachineClass;
    TechTier tier;

    std::vector<ResourceAmount> inputs;
    std::vector<ResourceAmount> outputs;
    std::vector<ResourceAmount> byproducts;

    float durationSeconds = 1.0f;
    float powerKw = 0.0f;
};
```

### 4.4 Tech Tree

The tech tree controls progression. It decides which resources, machine classes, and recipes are available at each point.

Progression should be structured around production capability rather than arbitrary levels.

Example tech progression:

```text
Tier 0: Basic ore processing
Tier 1: Washing and waste handling
Tier 2: Fluids and gases
Tier 3: Chemical processing
Tier 4: Advanced refinement
Tier 5: Complex multi-output chains and optimization challenges
```

Tech unlocks should answer:

- What new machines become available?
- What new recipes become available?
- What new resources become relevant?
- What new objective types are introduced?

### 4.5 Scenario Objectives

A scenario defines the target production problem.

Example objectives:

```text
Produce 60 Iron Ingots/min.
Produce 20 Sulfuric Acid/min.
Produce 10 Advanced Alloy/min while generating less than 5 Waste/min.
Produce the target using no more than 8 machines.
```

Suggested objective data:

```cpp
struct ScenarioTarget
{
    ResourceId resourceId;
    float requiredRatePerMinute = 0.0f;
};

struct ScenarioDef
{
    ScenarioId id;
    std::string key;
    std::string displayName;
    TechTier tier;
    std::vector<ScenarioTarget> targets;
};
```

---

## 5. Graph Model

### 5.1 Graph Node Meaning

A graph node is a machine instance configured with a recipe.

A node is not merely "Crusher". It is:

```text
Machine: Crusher
Recipe: Crush Iron Ore
Inputs: Iron Ore
Outputs: Crushed Iron Ore
```

A machine may support multiple recipes. Selecting a recipe should generate or update that node's ports.

### 5.2 Ports

Ports represent recipe inputs and outputs.

Input ports are generated from recipe inputs. Output ports are generated from recipe outputs and byproducts.

Port compatibility is based on:

- direction: output to input only
- resource identity
- optionally resource kind, if wildcard ports are later added
- machine and recipe rules
- duplicate connection rules

### 5.3 Edges

Edges represent a resource flow from one output port to one input port.

An edge should eventually carry:

```cpp
struct EdgeFlow
{
    EdgeId edgeId;
    ResourceId resourceId;
    float requestedRatePerMinute = 0.0f;
    float actualRatePerMinute = 0.0f;
};
```

The graph editor should allow the player to build connections freely within validation constraints, while the evaluator determines whether the production rates satisfy requirements.

---

## 6. Production Evaluation

The evaluator is not just a metric summation pass. It is a production-chain resolver.

It should answer:

- Which target resources are satisfied?
- Which target resources are underproduced?
- Which nodes are starved for inputs?
- Which nodes are over-supplied?
- Which outputs are unused?
- Which byproducts are accumulating?
- Which connections are invalid?
- Which node is the bottleneck for a target chain?
- How much total power, waste, and complexity does the solution require?

Suggested result shape:

```cpp
struct ResourceFlowSummary
{
    ResourceId resourceId;
    float producedPerMinute = 0.0f;
    float consumedPerMinute = 0.0f;
    float surplusPerMinute = 0.0f;
    float deficitPerMinute = 0.0f;
};

struct TargetStatus
{
    ResourceId resourceId;
    float requiredPerMinute = 0.0f;
    float actualPerMinute = 0.0f;
    float satisfactionRatio = 0.0f;
};

struct NodeProductionEval
{
    NodeId nodeId;
    bool active = false;
    bool starved = false;
    bool bottleneck = false;
    float utilization = 0.0f;
    std::vector<std::string> messages;
};

struct ProductionEvaluation
{
    std::vector<ResourceFlowSummary> resources;
    std::vector<TargetStatus> targets;
    std::vector<NodeProductionEval> nodes;
    std::vector<std::string> events;

    float totalPowerKw = 0.0f;
    float totalWastePerMinute = 0.0f;
    float targetSatisfactionRatio = 0.0f;
    int bottleneckCount = 0;
    int invalidConnectionCount = 0;
};
```

The first implementation can be simple and conservative. It does not need a perfect general solver immediately. It should start with acyclic graphs and gradually add support for loops, recycling, storage, and alternate route selection.

---

## 7. User Interface Direction

### 7.1 UI References

The UI should be inspired by production graph editors and node-based creative tools, such as:

- Blender node editor
- Dynamo
- Unreal Blueprints
- Houdini networks
- Substance Designer
- TouchDesigner
- Node-RED

The UI should not copy static process diagrams. Those diagrams are references for recipe-chain structure only.

### 7.2 Primary UI Goals

The interface should prioritize:

1. Fast graph construction
2. Readable node chains
3. Clear port compatibility
4. Strong validation feedback
5. Search-first node creation
6. Understandable production deficits and bottlenecks
7. Progression visibility through tech unlocks

### 7.3 Major UI Regions

The current application layout can remain, but each region should be reframed.

| Region | Purpose |
|---|---|
| Top bar | Scenario, tech tier, target progress, save/load controls |
| Left panel | Objectives, unlocked recipes/machines, resource search |
| Center canvas | Main node graph editor |
| Right panel | Selected machine, recipe, or resource properties |
| Inspector | Validation details, deficits, bottlenecks, selected node/edge explanation |
| Event log | Solver feedback, unlocks, invalid connection reasons |

### 7.4 Node Creation UX

Node creation should be search-first.

Preferred interaction:

```text
Right-click or hotkey opens Add Node search.
Player types "crusher", "iron", "wash", or "smelt".
Search results show unlocked machines and recipes.
Selecting a result creates a configured node at the cursor.
```

Later, the player should also be able to work backward from a target resource:

```text
Right-click missing Iron Ingot input -> Show recipes that produce Iron Ingot.
Choose Smelt Washed Iron Ore -> Create compatible Smelter node.
```

---

## 8. MVP Content

The first true gameplay MVP should be small.

### Tier 0 Resources

- Iron Ore
- Crushed Iron Ore
- Washed Iron Ore
- Iron Slurry
- Iron Ingot
- Water
- Waste

### Tier 0 Machines

- Resource Source
- Crusher
- Washer
- Smelter
- Waste Sink

### Tier 0 Recipes

#### Extract Iron Ore

```text
Machine: Resource Source
Outputs:
- Iron Ore: 60/min
```

#### Supply Water

```text
Machine: Resource Source
Outputs:
- Water: 60/min
```

#### Crush Iron Ore

```text
Machine: Crusher
Inputs:
- Iron Ore: 60/min
Outputs:
- Crushed Iron Ore: 60/min
```

#### Wash Crushed Iron Ore

```text
Machine: Washer
Inputs:
- Crushed Iron Ore: 60/min
- Water: 30/min
Outputs:
- Washed Iron Ore: 50/min
Byproducts:
- Iron Slurry: 10/min
```

#### Smelt Washed Iron Ore

```text
Machine: Smelter
Inputs:
- Washed Iron Ore: 50/min
Outputs:
- Iron Ingot: 50/min
```

#### Store Iron Slurry

```text
Machine: Waste Sink
Inputs:
- Iron Slurry: 10/min
Outputs:
- Waste: 10/min
```

### Tier 0 Scenario

```text
Produce 50 Iron Ingots/min.
Handle all Iron Slurry byproducts.
```

### Tier 0 Puzzle Solution Shape

```text
Iron Ore Source -> Crusher -> Washer -> Smelter -> Iron Ingot target
Water Source ----------------^ 
Washer -> Iron Slurry -> Waste Sink
```

The player should learn that producing the target item is not enough if a required byproduct is unmanaged.

---

## 9. Technical Architecture

The custom C++ architecture remains valid.

The application should continue to separate:

```text
Renderer draws.
UI manages widgets and interaction.
Graph editor manages graph editing behavior.
Graph document stores graph data.
Catalogs define production content.
Evaluator interprets graph meaning.
Serializer persists graph and view state.
```

Suggested high-level system layout:

```text
Application
 ├─ Platform Layer
 ├─ Renderer2D
 ├─ UI System
 ├─ Graph Editor
 ├─ Production Catalogs
 │   ├─ ResourceCatalog
 │   ├─ MachineCatalog
 │   ├─ RecipeCatalog
 │   └─ TechTree
 ├─ Scenario System
 ├─ Production Evaluator
 └─ Save/Load Serializer
```

The existing implementation work remains valuable:

- SDL3 window and input
- OpenGL renderer
- text rendering
- graph canvas
- node dragging
- wire creation
- validation
- selection
- deletion
- save/load
- tests

The next architectural shift is to replace generic graph metrics with production catalog data and scenario target evaluation.

---

## 10. Near-Term Patch Roadmap

### Patch 033: Add production catalog foundation

Add resource, machine, and recipe catalog types with hardcoded Tier 0 data.

Expected files:

```text
src/graph/ProductionTypes.h
src/graph/ResourceCatalog.h
src/graph/ResourceCatalog.cpp
src/graph/MachineCatalog.h
src/graph/MachineCatalog.cpp
src/graph/RecipeCatalog.h
src/graph/RecipeCatalog.cpp
tests/ProductionCatalogTest.cpp
```

### Patch 034: Attach recipes to graph nodes

Allow graph nodes to reference machine and recipe IDs. Generate node ports from recipe inputs, outputs, and byproducts.

### Patch 035: Replace sample graph with recipe-driven Tier 0 chain

Replace the current sample factory graph with a graph built from catalog definitions.

### Patch 036: Add scenario objective foundation

Add scenario targets such as "Produce 50 Iron Ingots/min" and "Handle all Iron Slurry."

### Patch 037: Add production target evaluator

Evaluate produced resources, consumed resources, deficits, surplus, byproducts, and target satisfaction.

### Patch 038: Feed UI panels from production evaluation

Replace plant-dashboard metrics with objective progress, deficits, bottlenecks, waste/byproduct status, and recipe-chain validation.

### Patch 039: Add Add Node search menu foundation

Start a search-first node creation workflow inspired by node graph tools.

### Patch 040: Add recipe selection for machine nodes

Allow supported machines to switch between unlocked recipes.

---

## 11. Design Non-Goals for Now

Do not build these yet:

- Full physical factory placement
- Belts, terrain, collision, or pathfinding
- Multiplayer
- A perfect production solver
- Full mod support
- Complex cyclic graph solving
- Large tech tree content
- Advanced animation and visual polish

The immediate goal is to prove the recipe graph puzzle loop.

---

## 12. Success Criteria

The next major milestone is successful when the player can:

1. View a target objective.
2. Place or load recipe-driven machine nodes.
3. Connect ports using resource-compatible edges.
4. See whether the target resource is produced at the required rate.
5. See which inputs are missing or under-supplied.
6. See which byproducts are unmanaged.
7. Modify the graph until the scenario is satisfied.
8. Save and reload the graph with view and selection state intact.

That milestone turns Crushline from a node UI prototype into the beginning of the actual game.
