#include "ProcessTreeModel.hpp"
#include <QtCore/QFileInfo>
#include <unordered_set>
#include <cmath>

namespace pm::gui {

ProcessTreeModel::ProcessTreeModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_rootItem(std::make_unique<ProcessTreeItem>(pm::core::ProcessSnapshot{}))
{
}

QModelIndex ProcessTreeModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) {
        return {};
    }

    ProcessTreeItem* parentItem = itemFromIndex(parent);
    ProcessTreeItem* childItem = parentItem ? parentItem->child(row) : nullptr;
    if (childItem) {
        return createIndex(row, column, childItem);
    }
    return {};
}

QModelIndex ProcessTreeModel::parent(const QModelIndex& index) const {
    if (!index.isValid()) {
        return {};
    }

    ProcessTreeItem* childItem = itemFromIndex(index);
    ProcessTreeItem* parentItem = childItem ? childItem->parentItem() : nullptr;

    if (!parentItem || parentItem == m_rootItem.get()) {
        return {};
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int ProcessTreeModel::rowCount(const QModelIndex& parent) const {
    ProcessTreeItem* parentItem = itemFromIndex(parent);
    return parentItem ? parentItem->childCount() : 0;
}

int ProcessTreeModel::columnCount(const QModelIndex& /*parent*/) const {
    return ColumnCount;
}

ProcessTreeItem* ProcessTreeModel::itemFromIndex(const QModelIndex& index) const noexcept {
    if (index.isValid()) {
        return static_cast<ProcessTreeItem*>(index.internalPointer());
    }
    return m_rootItem.get();
}

QModelIndex ProcessTreeModel::indexForItem(ProcessTreeItem* item, int column) const {
    if (!item || item == m_rootItem.get()) {
        return {};
    }
    return createIndex(item->row(), column, item);
}

const pm::core::ProcessSnapshot* ProcessTreeModel::processAt(const QModelIndex& index) const noexcept {
    if (!index.isValid()) {
        return nullptr;
    }
    ProcessTreeItem* item = itemFromIndex(index);
    return item ? &item->snapshot() : nullptr;
}

QModelIndex ProcessTreeModel::indexForPid(pm::core::ProcessId pid) const {
    auto it = m_itemMap.find(pid);
    if (it != m_itemMap.end()) {
        return indexForItem(it->second, 0);
    }
    return {};
}

QString ProcessTreeModel::formatBytes(std::uint64_t bytes) {
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;
    constexpr double GB = MB * 1024.0;

    const auto dBytes = static_cast<double>(bytes);
    if (dBytes >= GB) {
        return QString::asprintf("%.2f GB", dBytes / GB);
    }
    if (dBytes >= MB) {
        return QString::asprintf("%.1f MB", dBytes / MB);
    }
    if (dBytes >= KB) {
        return QString::asprintf("%.0f KB", dBytes / KB);
    }
    return QString::asprintf("%llu B", static_cast<unsigned long long>(bytes));
}

QString ProcessTreeModel::formatPriority(pm::core::ProcessPriority priority) {
    switch (priority) {
        case pm::core::ProcessPriority::Idle:        return tr("Низкий (Idle)");
        case pm::core::ProcessPriority::BelowNormal: return tr("Ниже обычного");
        case pm::core::ProcessPriority::Normal:      return tr("Обычный");
        case pm::core::ProcessPriority::AboveNormal: return tr("Выше обычного");
        case pm::core::ProcessPriority::High:        return tr("Высокий");
        case pm::core::ProcessPriority::RealTime:    return tr("Реального времени");
        default:                                     return tr("Неизвестно");
    }
}

QVariant ProcessTreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return {};
    }

    ProcessTreeItem* item = itemFromIndex(index);
    if (!item) {
        return {};
    }

    const auto& s = item->snapshot();

    // Пользовательские роли для делегатов и сортировки
    if (role == LifecycleRole) {
        return static_cast<int>(s.lifecycleState);
    }
    if (role == LifecycleCounterRole) {
        return s.lifecycleCounter;
    }
    if (role == PidRole) {
        return s.pid;
    }

    if (role == RawValueRole) {
        switch (index.column()) {
            case ColName:         return QString::fromStdString(s.name);
            case ColPid:          return s.pid;
            case ColCpu:          return s.cpuUsagePercent;
            case ColWorkingSet:   return static_cast<qulonglong>(s.workingSetBytes);
            case ColPrivateBytes: return static_cast<qulonglong>(s.privateBytes);
            case ColThreads:      return s.threadCount;
            case ColHandles:      return s.handleCount;
            case ColIoRead:       return static_cast<qulonglong>(s.readBytesPerSec);
            case ColIoWrite:      return static_cast<qulonglong>(s.writeBytesPerSec);
            case ColPriority:     return static_cast<int>(s.priority);
            case ColPath:         return QString::fromStdString(s.exePath);
            default:              return {};
        }
    }

    // Отображаемый текст ячейки
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColName:
                return QString::fromStdString(s.name);
            case ColPid:
                return QString::number(s.pid);
            case ColCpu:
                return QString::asprintf("%.1f%%", s.cpuUsagePercent);
            case ColWorkingSet:
                return formatBytes(s.workingSetBytes);
            case ColPrivateBytes:
                return formatBytes(s.privateBytes);
            case ColThreads:
                return QString::number(s.threadCount);
            case ColHandles:
                return QString::number(s.handleCount);
            case ColIoRead:
                return s.readBytesPerSec > 0 ? formatBytes(s.readBytesPerSec) + "/s" : QString("-");
            case ColIoWrite:
                return s.writeBytesPerSec > 0 ? formatBytes(s.writeBytesPerSec) + "/s" : QString("-");
            case ColPriority:
                return formatPriority(s.priority);
            case ColPath:
                return QString::fromStdString(s.exePath);
            default:
                return {};
        }
    }

    if (role == Qt::ToolTipRole) {
        return tr("PID: %1\nПуть: %2\nКоманда: %3")
            .arg(s.pid)
            .arg(QString::fromStdString(s.exePath))
            .arg(QString::fromStdString(s.commandLine));
    }

    return {};
}

QVariant ProcessTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case ColName:         return tr("Имя процесса");
            case ColPid:          return tr("PID");
            case ColCpu:          return tr("ЦП (%)");
            case ColWorkingSet:   return tr("Память (RSS)");
            case ColPrivateBytes: return tr("Приватная память");
            case ColThreads:      return tr("Потоки");
            case ColHandles:      return tr("Дескрипторы");
            case ColIoRead:       return tr("Чтение I/O");
            case ColIoWrite:      return tr("Запись I/O");
            case ColPriority:     return tr("Приоритет");
            case ColPath:         return tr("Путь к файлу");
            default:              return {};
        }
    }
    return {};
}

Qt::ItemFlags ProcessTreeModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void ProcessTreeModel::updateProcesses(std::vector<pm::core::ProcessSnapshot> processes) {
    std::unordered_map<pm::core::ProcessId, const pm::core::ProcessSnapshot*> incomingMap;
    incomingMap.reserve(processes.size());
    for (const auto& proc : processes) {
        incomingMap[proc.pid] = &proc;
    }

    // 1. Поиск и удаление завершившихся процессов
    std::vector<pm::core::ProcessId> pidsToRemove;
    for (const auto& [pid, item] : m_itemMap) {
        if (!incomingMap.contains(pid)) {
            pidsToRemove.push_back(pid);
        }
    }

    for (const auto pid : pidsToRemove) {
        auto it = m_itemMap.find(pid);
        if (it == m_itemMap.end()) continue;

        ProcessTreeItem* item = it->second;
        ProcessTreeItem* parentItem = item->parentItem() ? item->parentItem() : m_rootItem.get();
        const int row = item->row();
        const QModelIndex parentIndex = indexForItem(parentItem == m_rootItem.get() ? nullptr : parentItem, 0);

        beginRemoveRows(parentIndex, row, row);
        m_itemMap.erase(it);
        parentItem->removeChild(row);
        endRemoveRows();
    }

    // 2. Добавление новых и точечное обновление существующих процессов
    for (auto& proc : processes) {
        auto it = m_itemMap.find(proc.pid);
        if (it != m_itemMap.end()) {
            // Процесс уже существует в дереве -> обновляем поля и сигнализируем изменения
            ProcessTreeItem* item = it->second;
            item->updateSnapshot(std::move(proc));

            const QModelIndex leftIndex = indexForItem(item, 0);
            const QModelIndex rightIndex = indexForItem(item, ColumnCount - 1);
            emit dataChanged(leftIndex, rightIndex, {Qt::DisplayRole, RawValueRole, LifecycleRole, LifecycleCounterRole});
        } else {
            // Новый процесс -> определяем родительский узел
            ProcessTreeItem* parentItem = m_rootItem.get();
            if (proc.parentPid != 0 && proc.parentPid != proc.pid) {
                auto itParent = m_itemMap.find(proc.parentPid);
                if (itParent != m_itemMap.end()) {
                    parentItem = itParent->second;
                }
            }

            const int newRow = parentItem->childCount();
            const QModelIndex parentIndex = indexForItem(parentItem == m_rootItem.get() ? nullptr : parentItem, 0);

            beginInsertRows(parentIndex, newRow, newRow);
            const auto pid = proc.pid;
            auto newItem = std::make_unique<ProcessTreeItem>(std::move(proc), parentItem);
            m_itemMap[pid] = newItem.get();
            parentItem->appendChild(std::move(newItem));
            endInsertRows();
        }
    }
}

} // namespace pm::gui
