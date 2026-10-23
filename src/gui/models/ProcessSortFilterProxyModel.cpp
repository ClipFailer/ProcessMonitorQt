#include "ProcessSortFilterProxyModel.hpp"
#include "ProcessTreeModel.hpp"

namespace pm::gui {

ProcessSortFilterProxyModel::ProcessSortFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void ProcessSortFilterProxyModel::setSearchPattern(const QString& pattern) {
    m_filterPattern = pattern.trimmed();
    invalidateFilter();
}

bool ProcessSortFilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
    const QVariant leftVal = sourceModel()->data(source_left, ProcessTreeModel::RawValueRole);
    const QVariant rightVal = sourceModel()->data(source_right, ProcessTreeModel::RawValueRole);

    if (leftVal.isValid() && rightVal.isValid()) {
        // Числовое сравнение с плавающей точкой (для CPU %)
        if (leftVal.userType() == QMetaType::Double) {
            return leftVal.toDouble() < rightVal.toDouble();
        }
        // Числовое сравнение беззнаковых 64-битных целых (для байтов памяти и I/O)
        if (leftVal.userType() == QMetaType::ULongLong) {
            return leftVal.toULongLong() < rightVal.toULongLong();
        }
        // Числовое сравнение целых (для PID, потоков, дескрипторов)
        if (leftVal.userType() == QMetaType::UInt || leftVal.userType() == QMetaType::Int) {
            return leftVal.toLongLong() < rightVal.toLongLong();
        }
        // Строковое сравнение
        if (leftVal.userType() == QMetaType::QString) {
            return QString::localeAwareCompare(leftVal.toString(), rightVal.toString()) < 0;
        }
    }

    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

bool ProcessSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
    if (m_filterPattern.isEmpty()) {
        return true;
    }

    // Проверяем имя процесса (колонка 0)
    const QModelIndex nameIndex = sourceModel()->index(source_row, ProcessTreeModel::ColName, source_parent);
    const QString name = sourceModel()->data(nameIndex, Qt::DisplayRole).toString();
    if (name.contains(m_filterPattern, Qt::CaseInsensitive)) {
        return true;
    }

    // Проверяем PID (колонка 1)
    const QModelIndex pidIndex = sourceModel()->index(source_row, ProcessTreeModel::ColPid, source_parent);
    const QString pidStr = sourceModel()->data(pidIndex, Qt::DisplayRole).toString();
    if (pidStr.contains(m_filterPattern, Qt::CaseInsensitive)) {
        return true;
    }

    // Иерархическая проверка: если хотя бы один потомок подходит под фильтр, оставляем родителя
    return hasAcceptedChildren(source_row, source_parent);
}

bool ProcessSortFilterProxyModel::hasAcceptedChildren(int source_row, const QModelIndex& source_parent) const {
    const QModelIndex itemIndex = sourceModel()->index(source_row, 0, source_parent);
    if (!itemIndex.isValid()) {
        return false;
    }

    const int childCount = sourceModel()->rowCount(itemIndex);
    for (int i = 0; i < childCount; ++i) {
        if (filterAcceptsRow(i, itemIndex)) {
            return true;
        }
    }
    return false;
}

} // namespace pm::gui
