#pragma once

#include <QtCore/QSortFilterProxyModel>

namespace pm::gui {

/**
 * @brief Прокси-модель для быстрой сортировки и многоуровневой фильтрации дерева процессов.
 * 
 * Особенности:
 * 1. Числовая сортировка: столбцы (PID, ЦП, Память, Потоки, I/O) сортируются по сырым числам,
 *    а не по отформатированным строкам ("100 MB" vs "2 GB").
 * 2. Иерархический поиск: если дочерний узел удовлетворяет поисковому запросу,
 *    все его родительские узлы также отображаются в представлении.
 */
class ProcessSortFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit ProcessSortFilterProxyModel(QObject* parent = nullptr);
    ~ProcessSortFilterProxyModel() override = default;

    /**
     * @brief Задать строку поискового фильтра (по имени процесса или PID).
     * @param[in] pattern Текст фильтра.
     */
    void setSearchPattern(const QString& pattern);

protected:
    [[nodiscard]] bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
    [[nodiscard]] bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    [[nodiscard]] bool hasAcceptedChildren(int source_row, const QModelIndex& source_parent) const;

    QString m_filterPattern;
};

} // namespace pm::gui
