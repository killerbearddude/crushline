// Verifies the Crushline-owned production graph metadata layer. These tests
// protect the bridge between generic graph IDs and game meaning before WNG is
// introduced as the reusable graph engine.

#include "production/ProductionGraphMetadata.h"

#include <cmath>
#include <iostream>

namespace
{
bool Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Production graph metadata test failed: " << message << "\n";
        return false;
    }

    return true;
}

bool NearlyEqual(float left, float right)
{
    return std::fabs(left - right) < 0.001f;
}

bool CheckStrongIdsAndDefaults()
{
    // Invalid zero IDs mirror the current graph document conventions and prevent
    // accidental metadata records that cannot be synchronized with graph objects.
    return
        Check(!production::GraphNodeId{}.IsValid(), "default node id should be invalid") &&
        Check(!production::GraphPortId{}.IsValid(), "default port id should be invalid") &&
        Check(!production::GraphLinkId{}.IsValid(), "default link id should be invalid") &&
        Check(!production::ProductionMachineId{}.IsValid(), "default machine id should be invalid") &&
        Check(!production::ProductionRecipeId{}.IsValid(), "default recipe id should be invalid") &&
        Check(!production::ProductionResourceId{}.IsValid(), "default resource id should be invalid") &&
        Check(production::GraphNodeId{1}.IsValid(), "positive node id should be valid") &&
        Check(production::GraphPortId{2}.IsValid(), "positive port id should be valid") &&
        Check(production::GraphLinkId{3}.IsValid(), "positive link id should be valid");
}

bool CheckNodeUpsertAndLookup()
{
    // Verifies that node metadata can be inserted and replaced without depending
    // on catalog or evaluator classes. This keeps the metadata layer isolated.
    production::ProductionGraphMetadata metadata;

    if (!Check(metadata.Empty(), "new metadata container should be empty") ||
        !Check(!metadata.UpsertNode({}), "invalid node metadata should be rejected"))
    {
        return false;
    }

    const production::GraphNodeId graphNodeId{10};
    const bool inserted = metadata.UpsertNode({
        .graphNodeId = graphNodeId,
        .machineId = production::ProductionMachineId{20},
        .recipeId = production::ProductionRecipeId{30},
        .enabled = true
    });

    const production::ProductionNodeMetadata* storedNode = metadata.FindNode(graphNodeId);
    if (!Check(inserted, "valid node metadata should insert") ||
        !Check(storedNode != nullptr, "inserted node metadata should be findable"))
    {
        return false;
    }

    const bool replaced = metadata.UpsertNode({
        .graphNodeId = graphNodeId,
        .machineId = production::ProductionMachineId{21},
        .recipeId = std::nullopt,
        .enabled = false
    });

    storedNode = metadata.FindNode(graphNodeId);

    return
        Check(replaced, "matching node metadata should replace") &&
        Check(metadata.NodeCount() == 1, "node replacement should not add a duplicate") &&
        Check(storedNode != nullptr, "replaced node metadata should remain findable") &&
        Check(storedNode->machineId == production::ProductionMachineId{21}, "replacement should update machine id") &&
        Check(!storedNode->recipeId.has_value(), "replacement should allow absent recipe id") &&
        Check(!storedNode->enabled, "replacement should update enabled state");
}

bool CheckPortAndLinkMetadata()
{
    // Verifies that ports and links can carry Crushline-owned resource/rate
    // meaning while remaining structurally identified by graph IDs.
    production::ProductionGraphMetadata metadata;

    const production::GraphNodeId fromNodeId{1};
    const production::GraphNodeId toNodeId{2};
    const production::GraphPortId fromPortId{10};
    const production::GraphPortId toPortId{20};
    const production::GraphLinkId graphLinkId{30};
    const production::ProductionResourceId ironOreId{40};

    const bool portInserted =
        metadata.UpsertPort({
            .graphNodeId = fromNodeId,
            .graphPortId = fromPortId,
            .resourceId = ironOreId,
            .role = production::ProductionPortRole::Byproduct,
            .declaredRatePerMinute = 12.5f
        }) &&
        metadata.UpsertPort({
            .graphNodeId = toNodeId,
            .graphPortId = toPortId,
            .resourceId = ironOreId,
            .role = production::ProductionPortRole::Input,
            .declaredRatePerMinute = 10.0f
        });

    const bool linkInserted = metadata.UpsertLink({
        .graphLinkId = graphLinkId,
        .fromPortId = fromPortId,
        .toPortId = toPortId,
        .resourceId = ironOreId,
        .requestedRatePerMinute = 8.0f,
        .actualRatePerMinute = 6.5f
    });

    const production::ProductionPortMetadata* fromPort = metadata.FindPort(fromPortId);
    const production::ProductionLinkMetadata* link = metadata.FindLink(graphLinkId);

    return
        Check(portInserted, "valid port metadata should insert") &&
        Check(linkInserted, "valid link metadata should insert") &&
        Check(fromPort != nullptr, "source port metadata should be findable") &&
        Check(link != nullptr, "link metadata should be findable") &&
        Check(metadata.PortCount() == 2, "two port metadata records should be stored") &&
        Check(metadata.LinkCount() == 1, "one link metadata record should be stored") &&
        Check(fromPort->role == production::ProductionPortRole::Byproduct, "port role should be preserved") &&
        Check(fromPort->resourceId == ironOreId, "port resource id should be preserved") &&
        Check(NearlyEqual(fromPort->declaredRatePerMinute, 12.5f), "port declared rate should be preserved") &&
        Check(link->resourceId == ironOreId, "link resource id should be preserved") &&
        Check(NearlyEqual(link->requestedRatePerMinute, 8.0f), "link requested rate should be preserved") &&
        Check(NearlyEqual(link->actualRatePerMinute, 6.5f), "link actual rate should be preserved");
}

bool CheckNodeRemovalCascadesStaleMetadata()
{
    // Removing a graph node must also remove production metadata for its ports and
    // any interpreted links that touch those ports. This prevents stale game
    // meaning from surviving after graph structure changes.
    production::ProductionGraphMetadata metadata;

    const production::GraphNodeId removedNodeId{1};
    const production::GraphNodeId remainingNodeId{2};
    const production::GraphPortId removedPortId{10};
    const production::GraphPortId remainingPortId{20};
    const production::GraphLinkId removedLinkId{30};

    metadata.UpsertNode({removedNodeId, production::ProductionMachineId{100}, production::ProductionRecipeId{200}, true});
    metadata.UpsertNode({remainingNodeId, production::ProductionMachineId{101}, std::nullopt, true});
    metadata.UpsertPort({removedNodeId, removedPortId, production::ProductionResourceId{300}, production::ProductionPortRole::Output, 15.0f});
    metadata.UpsertPort({remainingNodeId, remainingPortId, production::ProductionResourceId{300}, production::ProductionPortRole::Input, 15.0f});
    metadata.UpsertLink({removedLinkId, removedPortId, remainingPortId, production::ProductionResourceId{300}, 15.0f, 15.0f});

    const bool removed = metadata.RemoveNode(removedNodeId);

    return
        Check(removed, "existing node metadata should remove") &&
        Check(metadata.FindNode(removedNodeId) == nullptr, "removed node metadata should be absent") &&
        Check(metadata.FindPort(removedPortId) == nullptr, "removed node port metadata should be absent") &&
        Check(metadata.FindLink(removedLinkId) == nullptr, "link touching removed node port should be absent") &&
        Check(metadata.FindNode(remainingNodeId) != nullptr, "unrelated node metadata should remain") &&
        Check(metadata.FindPort(remainingPortId) != nullptr, "unrelated port metadata should remain") &&
        Check(metadata.NodeCount() == 1, "one node metadata record should remain") &&
        Check(metadata.PortCount() == 1, "one port metadata record should remain") &&
        Check(metadata.LinkCount() == 0, "no link metadata records should remain");
}

bool CheckPortAndLinkRemoval()
{
    // Verifies explicit port/link removal paths used by future graph mutation
    // synchronization code when a single port or link changes independently.
    production::ProductionGraphMetadata metadata;

    const production::GraphNodeId graphNodeId{1};
    const production::GraphPortId fromPortId{10};
    const production::GraphPortId toPortId{20};
    const production::GraphLinkId graphLinkId{30};

    metadata.UpsertPort({graphNodeId, fromPortId, std::nullopt, production::ProductionPortRole::Service, 0.0f});
    metadata.UpsertPort({graphNodeId, toPortId, std::nullopt, production::ProductionPortRole::Service, 0.0f});
    metadata.UpsertLink({graphLinkId, fromPortId, toPortId, std::nullopt, 0.0f, 0.0f});

    const bool missingPortRemoved = metadata.RemovePort(production::GraphPortId{999});
    const bool removedPort = metadata.RemovePort(fromPortId);
    const bool missingLinkRemoved = metadata.RemoveLink(graphLinkId);

    return
        Check(!missingPortRemoved, "missing port removal should report false") &&
        Check(removedPort, "existing port metadata should remove") &&
        Check(metadata.FindPort(fromPortId) == nullptr, "removed port metadata should be absent") &&
        Check(metadata.FindLink(graphLinkId) == nullptr, "link touching removed port should be absent") &&
        Check(!missingLinkRemoved, "already cascaded link removal should report false") &&
        Check(metadata.PortCount() == 1, "one service port metadata record should remain");
}
}

int RunProductionGraphMetadataTest()
{
    if (!CheckStrongIdsAndDefaults() ||
        !CheckNodeUpsertAndLookup() ||
        !CheckPortAndLinkMetadata() ||
        !CheckNodeRemovalCascadesStaleMetadata() ||
        !CheckPortAndLinkRemoval())
    {
        return 1;
    }

    std::cout << "Production graph metadata test passed.\n";
    return 0;
}
