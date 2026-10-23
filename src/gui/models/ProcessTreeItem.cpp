#include "ProcessTreeItem.hpp"
#include <algorithm>

namespace pm::gui {

ProcessTreeItem::ProcessTreeItem(pm::core::ProcessSnapshot snapshot, ProcessTreeItem* parent)
    : m_snapshot(std::move(snapshot))
    , m_parentItem(parent)
{
}

void ProcessTreeItem::appendChild(std::unique_ptr<ProcessTreeItem> child) {
    if (child) {
        child->m_parentItem = this;
        m_childItems.push_back(std::move(child));
    }
}

std::unique_ptr<ProcessTreeItem> ProcessTreeItem::removeChild(int row) {
    if (row < 0 || row >= static_cast<int>(m_childItems.size())) {
        return nullptr;
    }
    auto it = m_childItems.begin() + row;
    std::unique_ptr<ProcessTreeItem> removed = std::move(*it);
    m_childItems.erase(it);
    return removed;
}

ProcessTreeItem* ProcessTreeItem::child(int row) const noexcept {
    if (row < 0 || row >= static_cast<int>(m_childItems.size())) {
        return nullptr;
    }
    return m_childItems[static_cast<std::size_t>(row)].get();
}

int ProcessTreeItem::childCount() const noexcept {
    return static_cast<int>(m_childItems.size());
}

ProcessTreeItem* ProcessTreeItem::parentItem() const noexcept {
    return m_parentItem;
}

int ProcessTreeItem::row() const noexcept {
    if (!m_parentItem) {
        return 0;
    }
    const auto& siblings = m_parentItem->m_childItems;
    for (std::size_t i = 0; i < siblings.size(); ++i) {
        if (siblings[i].get() == this) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

const pm::core::ProcessSnapshot& ProcessTreeItem::snapshot() const noexcept {
    return m_snapshot;
}

void ProcessTreeItem::updateSnapshot(pm::core::ProcessSnapshot newSnapshot) noexcept {
    m_snapshot = std::move(newSnapshot);
}

} // namespace pm::gui
