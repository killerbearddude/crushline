// Implements storage and lookup for Crushline production graph metadata.
// This file intentionally avoids renderer, SDL, serializer, and evaluator
// dependencies so the metadata bridge can evolve independently from UI code.

#include "production/ProductionGraphMetadata.h"

#include <algorithm>
#include <vector>

namespace production
{
namespace
{
[[nodiscard]] bool ContainsPortId(const std::vector<GraphPortId>& portIds, GraphPortId graphPortId)
{
    return std::any_of(portIds.begin(), portIds.end(), [graphPortId](GraphPortId removedPortId) {
        return removedPortId == graphPortId;
    });
}
}

void ProductionGraphMetadata::Clear()
{
    m_nodes.clear();
    m_ports.clear();
    m_links.clear();
}

bool ProductionGraphMetadata::Empty() const
{
    return m_nodes.empty() && m_ports.empty() && m_links.empty();
}

std::size_t ProductionGraphMetadata::NodeCount() const
{
    return m_nodes.size();
}

std::size_t ProductionGraphMetadata::PortCount() const
{
    return m_ports.size();
}

std::size_t ProductionGraphMetadata::LinkCount() const
{
    return m_links.size();
}

bool ProductionGraphMetadata::UpsertNode(ProductionNodeMetadata metadata)
{
    if (!metadata.IsValid())
    {
        return false;
    }

    auto nodeIt = std::find_if(m_nodes.begin(), m_nodes.end(), [metadata](const ProductionNodeMetadata& node) {
        return node.graphNodeId == metadata.graphNodeId;
    });

    if (nodeIt != m_nodes.end())
    {
        *nodeIt = metadata;
        return true;
    }

    m_nodes.push_back(metadata);
    return true;
}

bool ProductionGraphMetadata::UpsertPort(ProductionPortMetadata metadata)
{
    if (!metadata.IsValid())
    {
        return false;
    }

    auto portIt = std::find_if(m_ports.begin(), m_ports.end(), [metadata](const ProductionPortMetadata& port) {
        return port.graphPortId == metadata.graphPortId;
    });

    if (portIt != m_ports.end())
    {
        *portIt = metadata;
        return true;
    }

    m_ports.push_back(metadata);
    return true;
}

bool ProductionGraphMetadata::UpsertLink(ProductionLinkMetadata metadata)
{
    if (!metadata.IsValid())
    {
        return false;
    }

    auto linkIt = std::find_if(m_links.begin(), m_links.end(), [metadata](const ProductionLinkMetadata& link) {
        return link.graphLinkId == metadata.graphLinkId;
    });

    if (linkIt != m_links.end())
    {
        *linkIt = metadata;
        return true;
    }

    m_links.push_back(metadata);
    return true;
}

const ProductionNodeMetadata* ProductionGraphMetadata::FindNode(GraphNodeId graphNodeId) const
{
    const auto nodeIt = std::find_if(m_nodes.begin(), m_nodes.end(), [graphNodeId](const ProductionNodeMetadata& node) {
        return node.graphNodeId == graphNodeId;
    });

    return nodeIt == m_nodes.end() ? nullptr : &(*nodeIt);
}

const ProductionPortMetadata* ProductionGraphMetadata::FindPort(GraphPortId graphPortId) const
{
    const auto portIt = std::find_if(m_ports.begin(), m_ports.end(), [graphPortId](const ProductionPortMetadata& port) {
        return port.graphPortId == graphPortId;
    });

    return portIt == m_ports.end() ? nullptr : &(*portIt);
}

const ProductionLinkMetadata* ProductionGraphMetadata::FindLink(GraphLinkId graphLinkId) const
{
    const auto linkIt = std::find_if(m_links.begin(), m_links.end(), [graphLinkId](const ProductionLinkMetadata& link) {
        return link.graphLinkId == graphLinkId;
    });

    return linkIt == m_links.end() ? nullptr : &(*linkIt);
}

bool ProductionGraphMetadata::RemoveNode(GraphNodeId graphNodeId)
{
    const auto previousNodeCount = m_nodes.size();
    m_nodes.erase(
        std::remove_if(m_nodes.begin(), m_nodes.end(), [graphNodeId](const ProductionNodeMetadata& node) {
            return node.graphNodeId == graphNodeId;
        }),
        m_nodes.end()
    );

    if (m_nodes.size() == previousNodeCount)
    {
        return false;
    }

    std::vector<GraphPortId> removedPortIds;
    for (const ProductionPortMetadata& port : m_ports)
    {
        if (port.graphNodeId == graphNodeId)
        {
            removedPortIds.push_back(port.graphPortId);
        }
    }

    m_ports.erase(
        std::remove_if(m_ports.begin(), m_ports.end(), [graphNodeId](const ProductionPortMetadata& port) {
            return port.graphNodeId == graphNodeId;
        }),
        m_ports.end()
    );

    // Link metadata references graph ports, so deleting a node must also remove
    // any link whose endpoint belonged to one of the removed port records.
    m_links.erase(
        std::remove_if(m_links.begin(), m_links.end(), [&removedPortIds](const ProductionLinkMetadata& link) {
            return
                ContainsPortId(removedPortIds, link.fromPortId) ||
                ContainsPortId(removedPortIds, link.toPortId);
        }),
        m_links.end()
    );

    return true;
}

bool ProductionGraphMetadata::RemovePort(GraphPortId graphPortId)
{
    const auto previousPortCount = m_ports.size();
    m_ports.erase(
        std::remove_if(m_ports.begin(), m_ports.end(), [graphPortId](const ProductionPortMetadata& port) {
            return port.graphPortId == graphPortId;
        }),
        m_ports.end()
    );

    if (m_ports.size() == previousPortCount)
    {
        return false;
    }

    m_links.erase(
        std::remove_if(m_links.begin(), m_links.end(), [graphPortId](const ProductionLinkMetadata& link) {
            return link.fromPortId == graphPortId || link.toPortId == graphPortId;
        }),
        m_links.end()
    );

    return true;
}

bool ProductionGraphMetadata::RemoveLink(GraphLinkId graphLinkId)
{
    const auto previousLinkCount = m_links.size();
    m_links.erase(
        std::remove_if(m_links.begin(), m_links.end(), [graphLinkId](const ProductionLinkMetadata& link) {
            return link.graphLinkId == graphLinkId;
        }),
        m_links.end()
    );

    return m_links.size() != previousLinkCount;
}

const std::vector<ProductionNodeMetadata>& ProductionGraphMetadata::Nodes() const
{
    return m_nodes;
}

const std::vector<ProductionPortMetadata>& ProductionGraphMetadata::Ports() const
{
    return m_ports;
}

const std::vector<ProductionLinkMetadata>& ProductionGraphMetadata::Links() const
{
    return m_links;
}
}
