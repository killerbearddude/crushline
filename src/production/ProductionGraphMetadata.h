// Defines Crushline-owned production metadata attached to generic graph
// elements. The graph layer owns structure; this layer records game meaning such
// as machines, recipes, resources, port roles, and interpreted link rates.

#pragma once

#include <cstddef>
#include <optional>
#include <vector>

namespace production
{
// Strong graph node reference used by Crushline metadata. The value mirrors the
// current graph document's positive integer IDs while keeping APIs explicit.
struct GraphNodeId
{
    int value = 0;

    [[nodiscard]] constexpr bool IsValid() const
    {
        return value > 0;
    }
};

// Strong graph port reference used by Crushline metadata. Invalid ID zero keeps
// parity with the existing graph document conventions.
struct GraphPortId
{
    int value = 0;

    [[nodiscard]] constexpr bool IsValid() const
    {
        return value > 0;
    }
};

// Strong graph link reference used by Crushline metadata. This is intentionally
// separate from port and node IDs to prevent accidental cross-use in call sites.
struct GraphLinkId
{
    int value = 0;

    [[nodiscard]] constexpr bool IsValid() const
    {
        return value > 0;
    }
};

// Crushline machine catalog reference. This type is kept lightweight so the
// metadata layer can be tested without depending on the current catalog classes.
struct ProductionMachineId
{
    int value = 0;

    [[nodiscard]] constexpr bool IsValid() const
    {
        return value > 0;
    }
};

// Crushline recipe catalog reference. Optional recipe IDs allow non-recipe
// machine nodes, buffers, or future utility nodes to participate in metadata.
struct ProductionRecipeId
{
    int value = 0;

    [[nodiscard]] constexpr bool IsValid() const
    {
        return value > 0;
    }
};

// Crushline resource catalog reference. Port and link metadata use this value as
// game meaning; generic graph structure must not interpret it as compatibility.
struct ProductionResourceId
{
    int value = 0;

    [[nodiscard]] constexpr bool IsValid() const
    {
        return value > 0;
    }
};

[[nodiscard]] constexpr bool operator==(GraphNodeId left, GraphNodeId right)
{
    return left.value == right.value;
}

[[nodiscard]] constexpr bool operator==(GraphPortId left, GraphPortId right)
{
    return left.value == right.value;
}

[[nodiscard]] constexpr bool operator==(GraphLinkId left, GraphLinkId right)
{
    return left.value == right.value;
}

[[nodiscard]] constexpr bool operator==(ProductionMachineId left, ProductionMachineId right)
{
    return left.value == right.value;
}

[[nodiscard]] constexpr bool operator==(ProductionRecipeId left, ProductionRecipeId right)
{
    return left.value == right.value;
}

[[nodiscard]] constexpr bool operator==(ProductionResourceId left, ProductionResourceId right)
{
    return left.value == right.value;
}

// Production-facing role for a graph port. These roles describe Crushline
// evaluator meaning only; structural direction remains a graph-engine concern.
enum class ProductionPortRole
{
    Input,
    Output,
    Byproduct,
    Waste,
    EnergyInput,
    EnergyOutput,
    Control,
    Service
};

// Metadata that maps one graph node to Crushline production meaning.
struct ProductionNodeMetadata
{
    GraphNodeId graphNodeId{};
    ProductionMachineId machineId{};
    std::optional<ProductionRecipeId> recipeId{};
    bool enabled = true;

    // Nodes require graph identity and machine meaning. A recipe remains optional
    // because not every future production node is expected to run a recipe.
    [[nodiscard]] constexpr bool IsValid() const
    {
        return graphNodeId.IsValid() && machineId.IsValid();
    }
};

// Metadata that maps one graph port to Crushline resource and role meaning.
struct ProductionPortMetadata
{
    GraphNodeId graphNodeId{};
    GraphPortId graphPortId{};
    std::optional<ProductionResourceId> resourceId{};
    ProductionPortRole role = ProductionPortRole::Input;
    float declaredRatePerMinute = 0.0f;

    // Ports require graph identity only. Resource meaning is optional for future
    // service, control, or energy ports that may not carry a material resource.
    [[nodiscard]] constexpr bool IsValid() const
    {
        return graphNodeId.IsValid() && graphPortId.IsValid();
    }
};

// Metadata that maps one graph link to interpreted production flow.
struct ProductionLinkMetadata
{
    GraphLinkId graphLinkId{};
    GraphPortId fromPortId{};
    GraphPortId toPortId{};
    std::optional<ProductionResourceId> resourceId{};
    float requestedRatePerMinute = 0.0f;
    float actualRatePerMinute = 0.0f;

    // Links require structural endpoint references. Resource and rate values are
    // evaluator-owned meaning and can remain empty or zero before evaluation.
    [[nodiscard]] constexpr bool IsValid() const
    {
        return graphLinkId.IsValid() && fromPortId.IsValid() && toPortId.IsValid();
    }
};

// Owns Crushline production metadata for graph nodes, ports, and links. The
// container deliberately does not mutate graph structure; callers keep this data
// synchronized when graph objects are created, reconfigured, or removed.
class ProductionGraphMetadata
{
public:
    // Removes all production metadata. Graph documents are cleared separately by
    // the graph layer or editor command that owns structural state.
    void Clear();

    // Returns true when no production metadata is currently stored.
    [[nodiscard]] bool Empty() const;

    // Returns the number of node metadata records.
    [[nodiscard]] std::size_t NodeCount() const;

    // Returns the number of port metadata records.
    [[nodiscard]] std::size_t PortCount() const;

    // Returns the number of link metadata records.
    [[nodiscard]] std::size_t LinkCount() const;

    // Inserts or replaces node metadata. Returns false when the record lacks the
    // minimum graph or production identity required for safe lookup.
    bool UpsertNode(ProductionNodeMetadata metadata);

    // Inserts or replaces port metadata. This does not require the node metadata
    // to already exist so import/load paths can restore records in any order.
    bool UpsertPort(ProductionPortMetadata metadata);

    // Inserts or replaces link metadata. Endpoint existence is not validated here
    // because structural validation remains owned by the graph layer.
    bool UpsertLink(ProductionLinkMetadata metadata);

    // Finds node metadata by graph node ID. The returned pointer is non-owning and
    // remains valid only until the next mutation of this metadata container.
    [[nodiscard]] const ProductionNodeMetadata* FindNode(GraphNodeId graphNodeId) const;

    // Finds port metadata by graph port ID. The returned pointer is non-owning and
    // remains valid only until the next mutation of this metadata container.
    [[nodiscard]] const ProductionPortMetadata* FindPort(GraphPortId graphPortId) const;

    // Finds link metadata by graph link ID. The returned pointer is non-owning and
    // remains valid only until the next mutation of this metadata container.
    [[nodiscard]] const ProductionLinkMetadata* FindLink(GraphLinkId graphLinkId) const;

    // Removes node metadata, every port owned by the node, and every link touching
    // a removed port. The cascade prevents stale production meaning from outliving
    // structural graph removals.
    bool RemoveNode(GraphNodeId graphNodeId);

    // Removes port metadata and every link touching that port. Returns false when
    // no matching port metadata exists.
    bool RemovePort(GraphPortId graphPortId);

    // Removes link metadata. Returns false when no matching link metadata exists.
    bool RemoveLink(GraphLinkId graphLinkId);

    // Exposes deterministic insertion-order records for tests, debug panels, and
    // future serializer work. Mutating callers should use the explicit APIs above.
    [[nodiscard]] const std::vector<ProductionNodeMetadata>& Nodes() const;
    [[nodiscard]] const std::vector<ProductionPortMetadata>& Ports() const;
    [[nodiscard]] const std::vector<ProductionLinkMetadata>& Links() const;

private:
    std::vector<ProductionNodeMetadata> m_nodes;
    std::vector<ProductionPortMetadata> m_ports;
    std::vector<ProductionLinkMetadata> m_links;
};
}
